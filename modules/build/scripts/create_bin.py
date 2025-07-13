import argparse
import struct
from elftools.elf.elffile import ELFFile
from elftools.elf.enums import ENUM_ST_INFO_TYPE

import zlib

args = None

HEADER_SIGNATURE = int.from_bytes(b'AIKM', byteorder='little')  # 'AIKa Module'
HEADER_SIZE = 64  # 48 header

def align_up(value, alignment):
    """Aligns value up to the nearest multiple of alignment."""
    return (value + alignment - 1) & ~(alignment - 1)

REL_EXPORTED = {'text': 1, 'data': 2, 'rodata': 3, 'bss': 4}

ARM_RELOC_MAP = {
    0: 'R_ARM_NONE',
    1: 'R_ARM_PC24',
    2: 'R_ARM_ABS32',
    3: 'R_ARM_REL32',
    10: 'R_ARM_THM_CALL',
    21: 'R_ARM_ABS32',
    23: 'R_ARM_REL32',
    45: 'R_ARM_THM_JUMP11',
    46: 'R_ARM_THM_MOVW_PREL_NC',
    47: 'R_ARM_THM_MOVW_ABS_NC',
    48: 'R_ARM_THM_MOVT_ABS',
    49: 'R_ARM_THM_MOVW_BF16',
    50: 'R_ARM_THM_MOVW_BF12',
    51: 'R_ARM_THM_MOVW_BA16',
    102: 'R_ARM_THM_ALU_PREV_INST',
    104: 'R_ARM_THM_PC12',
    105: 'R_ARM_THM_MOVW_PREL',
    106: 'R_ARM_THM_MOVT_PREL',
    108: 'R_ARM_THM_JUMP19',
}

def inc_ref_count(sym_table, sym_idx):
    for s in sym_table:
        if s['index'] == sym_idx:
            s['ref_count'] = s['ref_count'] + 1
            return

def inc_ref_count_dict(sym_dict, sym_idx):
    if sym_idx in sym_dict:
        sym_dict[sym_idx]['ref_count'] = sym_dict[sym_idx]['ref_count'] + 1

def type_to_string(type):
    if type in ARM_RELOC_MAP:
        return ARM_RELOC_MAP[type]
    return 'NONE'

def section_to_id(section):
    if section in REL_EXPORTED:
        return REL_EXPORTED[section]
    return 0

def get_symbol_position(symbols, sym_index):
    id = 0
    for idx, _ in symbols.items():
        if idx == sym_index:
            return id
        id = id + 1
    return -1

def show_section_list(elf):
    print("[*] Sections:")
    for idx, sec in enumerate(elf.iter_sections()):
        print(f"  [{idx}] {sec.name} (type: {sec['sh_type']}, flags: {sec['sh_flags']}, offset: {sec['sh_offset']}, addr: {hex(sec['sh_addr'])}, size: {sec['sh_size']})")
    print("[*] End of sections")

def parse_relocations(elf_path):
    with open(elf_path, 'rb') as f:
        elf = ELFFile(f)

        relocations = []
        symbols_map = {}

        entry_point = elf.header['e_entry']
        print(f"[*] ELF Entry Point: 0x{entry_point:08X}")

        bss_length = 0
        bss_start = 0

        # get bss section
        bss_sec = elf.get_section_by_name('.bss')
        if bss_sec:
            bss_length = bss_sec['sh_size']
            bss_start = bss_sec['sh_addr']
            print(f"[+] BSS section found: {bss_sec.name} (size: {bss_length}, start: {bss_start})")
        else:
            print("[!] No .bss section found")

        # parse symbols
        symtab_sec = elf.get_section_by_name('.symtab')
        if not symtab_sec:
            print("[!] No symbol table found")
            return

        if args.verbose:
            show_section_list(elf)

        for idx, sym in enumerate(symtab_sec.iter_symbols()):
            # if sym['st_info']['type'] == 'STT_SECTION':
            #     continue # sections are not needed

            symbols_map[idx] = {
                'value': sym['st_value'],
                'size': sym['st_size'],
                'name': sym.name,
                'type': sym['st_info']['type'],
                'type_val': ENUM_ST_INFO_TYPE.get(sym['st_info']['type'], -1),
                'section': sym['st_shndx'],
                'ref_count': 0,
            }


        # all sections. Get relocations
        for idx, sec in enumerate(elf.iter_sections()):

            sec_name = sec.name
            if(len(sec.name) == 0):
                continue
            if sec_name[0] == '.':
                sec_name = sec_name[1:]
            if sec_name in REL_EXPORTED:
                REL_EXPORTED[sec_name] = idx
                continue
                
            ssec = list(filter(lambda v: len(v) > 0, sec.name.split('.')))
            print(f"[+] Section: {sec.name} -> {ssec}")
            if len(ssec) > 1 and (ssec[0] == 'rel' or ssec[0] == 'rela') and ssec[1] in REL_EXPORTED:
                rel_section = sec
                symtab = elf.get_section(rel_section['sh_link'])
                target_sec = elf.get_section(rel_section['sh_info'])

                print(f"  [+] Found relocations in {rel_section.name} (applies to {target_sec.name})")

                for reloc in rel_section.iter_relocations():
                    offset = reloc['r_offset']
                    sym_index = reloc['r_info_sym']
                    reloc_type = reloc['r_info_type']

                    relocations.append({
                        'section': sec.name,
                        'idx': section_to_id(ssec[1]),
                        'offset': offset,
                        'type': reloc_type,
                        'stype': type_to_string(reloc_type),
                        'sym_index': sym_index,
                    })
                    inc_ref_count_dict(symbols_map, sym_index)

        print("[*] Symbols MAP:")
        for idx, sym in symbols_map.items():
            if sym['ref_count'] == 0:
                print(f"  [!] Symbol {sym['name']} (index {idx}) has no references. {sym}")
                continue
        symbols_map = {idx: sym for idx, sym in symbols_map.items() if sym.get('ref_count', 0) > 0}
        sym_values = []
        for idx, sym in symbols_map.items():
            value = struct.pack('<IIII', sym['value'], sym['section'], sym['type_val'], 0);
            sym_values.append(value)
            if args.verbose:
                print(f"  [*] {idx} -> {sym}")

        print("[*] Relocations:")
        rel_values = []
        for rel in relocations:
            sym_pos = get_symbol_position(symbols_map, rel['sym_index'])
            if sym_pos == -1:
                print(f"  [!] Symbol index {rel['sym_index']} not found in symbols map, skipping relocation.")
                continue
            value = struct.pack('<IIII', rel['offset'], rel['type'], rel['idx'], sym_pos);
            if args.verbose:
                print(f"  [*] {rel}")
            rel_values.append(value)

        return {'relocations': rel_values, 
                'symbols': sym_values, 
                'bss_start': bss_start, 
                'bss_length': bss_length,
                'entry_point': entry_point}

def main(): 
    parser = argparse.ArgumentParser(description="Prepare module binary with header and relocations.")

    # required arguments
    parser.add_argument('-e', '--elf', required=True,
                        help='Input ELF file (for section info and relocations)')

    parser.add_argument('-i', '--input-bin', required=True,
                        help='Raw input binary (after objcopy)')

    parser.add_argument('-o', '--output-bin', required=True,
                        help='Final output binary with header')

    # optional arguments
    parser.add_argument('--verbose', action='store_true',
                        help='Enable verbose mode')
    
    parser.add_argument('-d', '--description', nargs='?', default='', 
                        help='Module description (optional)')

    global args
    args = parser.parse_args()

    if args.verbose:
        print(f"[+] ELF file: {args.elf}")
        print(f"[+] Input bin: {args.input_bin}")
        print(f"[+] Output bin: {args.output_bin}")

        print("[*] Processing...")

    description = args.description.encode('utf-8')
    if len(description) > 32:
        print("[!] Description is too long, must be 32 bytes or less.")
        description = description[:31] 
        
    if len(description) > 0:
        description = description + b'\x00'

    descriprion_size = len(description);
    description_fixed_size = align_up(descriprion_size, 8) # align up to 8 bytes
    description_padding = description_fixed_size - descriprion_size

    if args.verbose:
        print(f"[*] Description: '{args.description}'")
        print(f"[*] Description size: {descriprion_size} bytes")
        print(f"[*] Fixed description size (aligned to 8 bytes): {description_fixed_size} bytes")

    entry_offset = HEADER_SIZE + description_fixed_size

    bin_data = b''
    with open(args.input_bin, "rb") as f:
        bin_data = f.read()
    bin_size = len(bin_data)
    fixed_bin_size = align_up(bin_size, 8)# align up to 8 bytes
    bin_size_padding = fixed_bin_size - bin_size

    binary_data = parse_relocations(args.elf)

    relocs = binary_data['relocations']
    symbols = binary_data['symbols']
    bss_offset = binary_data['bss_start']
    bss_length = binary_data['bss_length']
    entry_point = binary_data['entry_point']

    reloc_count = len(relocs)   # each relocation is 16 bytes
    symbols_count = len(symbols)  # each symbol is 16 bytes

    reloc_offset = entry_offset + fixed_bin_size
    symbols_offset = reloc_offset + (reloc_count * 16)

    header = struct.pack("<IIII IIII IIII IIII", 
                         HEADER_SIGNATURE,  # +0
                         (1 << 16) | (HEADER_SIZE & 0xFFFF), # +4 version + HEADER_SIZE 
                         entry_offset, fixed_bin_size, # +8, +12
                         reloc_offset, reloc_count, # +16, +20
                         symbols_offset, symbols_count, # +24, +28 
                         bss_offset, align_up(bss_length, 8), # + 32 +36 BSS length
                         0, # crs +40
                         0, # +44 total size 
                         entry_point, # +48 entry point offset
                         0,  # +52 reserved
                         0,  # +56 reserved
                         0,  # +60 reserved
    ) 
    if args.verbose:
        # Print header as 4-byte chunks in hex
        header_words = [header[i:i+4] for i in range(0, len(header), 4)]
        print("[*] Header:")
        for idx, word in enumerate(header_words):
            print(f"  [{idx:2}] {word.hex()}")

    total_size = 0
    with open(args.output_bin, "wb") as f:
        f.write(header)

        if f.tell() != HEADER_SIZE:
            print(f"[E] Header size mismatch: expected {HEADER_SIZE} bytes, got {f.tell()} bytes")
            return -1

        f.write(description)
        f.write(b'\x00' * description_padding)
        f.write(bin_data)
        f.write(b'\x00' * bin_size_padding)
        f.write(b''.join(relocs))
        f.write(b''.join(symbols))
        total_size = f.tell()
        if args.verbose:
            print(f"[*] Written header, description, binary, relocations and symbols to {args.output_bin}. Total size: {total_size} bytes") 

    if args.verbose:
        print(f"[+] Output bin: {args.output_bin}")
        print("[*] Processing completed successfully.")
        print(f"[*] Header size: {HEADER_SIZE} bytes")
        print(f"[*] Description size: {descriprion_size} bytes (padded to {description_fixed_size} bytes)")
        print(f"[*] Binary size: {bin_size} bytes (padded to {fixed_bin_size} bytes)")
        print(f"[*] Relocations size: {reloc_count} bytes")
        print(f"[*] Symbols size: {symbols_count} bytes")
    
    with open(args.output_bin, "r+b") as f:

        f.seek(44)
        f.write(struct.pack("<I", total_size))
        
        f.seek(0)
        data = f.read()
        
        crc32 = zlib.crc32(data) & 0xFFFFFFFF
        
        f.seek(40)
        f.write(struct.pack("<I", crc32))
        if args.verbose:
            print(f"[*] Calculated CRC32: {crc32:#010x}, total size: {len(data)} bytes")

if __name__ == "__main__":
    main()
