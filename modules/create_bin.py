import argparse
import struct
from elftools.elf.elffile import ELFFile
import zlib

HEADER_SIGNATURE = int.from_bytes(b'AIKM', byteorder='little')  # 'AIKa Module'
HEADER_SIZE = 32  # 32 header
ENTRY_OFFSET = 0x10  # default after header starts .text section

def align_up(value, alignment):
    """Aligns value up to the nearest multiple of alignment."""
    return (value + alignment - 1) & ~(alignment - 1)

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

    args = parser.parse_args()


    if args.verbose:
        print(f"[+] ELF file: {args.elf}")
        print(f"[+] Input bin: {args.input_bin}")
        print(f"[+] Output bin: {args.output_bin}")

        print("[*] Processing...")

    description = args.description.encode('utf-8')
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
    bin_size_padded = fixed_bin_size - bin_size
    
    text_size = 0
    data_size = 0
    bss_size = 0

    relocation_offsets = []

    with open(args.elf, 'rb') as f:
        print(f"[*] ELF file: {args.elf}...")
        elf = ELFFile(f)
        if args.verbose:
            print("[*] Scanning sections for relocations...")    
            for section in elf.iter_sections():
                print(f"[*]     Section: {section.name}, addr={hex(section['sh_addr'])}, size={section['sh_size']} bytes")

        text_section = elf.get_section_by_name('.text')
        if text_section:
            text_size = text_section['sh_size']
            if args.verbose:
                print(f"[*] .text section: addr={hex(text_section['sh_addr'])}, size={text_size} bytes")
                
        data_section = elf.get_section_by_name('.data')
        if data_section:
            data_size = data_section['sh_size']
            if args.verbose:
                print(f"[*] .data section: addr={hex(data_section['sh_addr'])}, size={data_size} bytes")

        bss_section = elf.get_section_by_name('.bss')
        bss_size = bss_section['sh_size'] if bss_section else 0
        bin_size = bin_size + bss_size 
        fixed_bin_size = align_up(bin_size, 8)
        bin_size_padded = fixed_bin_size - bin_size

        if args.verbose:
            print(f"[*] BSS section size: {bss_size} bytes")
            print(f"[*] Total binary size (with BSS): {bin_size} bytes")
            print(f"[*] Padded binary size: {fixed_bin_size} bytes")

        for section in elf.iter_sections():
            if not section.name.startswith('.rel'):
                continue
            for rel in section.iter_relocations():
                print(f"[*] Found relocation in section {section.name}: offset={hex(rel['r_offset'])} ({rel['r_offset']})")
                relocation_offsets.append(rel['r_offset'])

    relocation_offsets = sorted(set(relocation_offsets))
    relocation_table = b''.join([struct.pack("<I", offset) for offset in relocation_offsets])
    relocation_count = len(relocation_offsets)
    header = struct.pack("<IIIIIIII", 
                         HEADER_SIGNATURE,  # +0
                         fixed_bin_size,    # +4
                         entry_offset,      # +8
                         0,                 # +12 crc32 offset
                         0, 0,              # +16 reserved, +20 reserved
                         relocation_count,  # +24
                         HEADER_SIZE + fixed_bin_size + description_fixed_size + len(relocation_table))	# +28 total size

    with open(args.output_bin, "wb") as f:
        f.write(header)
        f.write(description)
        f.write(b'\x00' * description_padding)
        f.write(bin_data)
        f.write(b'\x00' * bss_size) if bss_size > 0 else None 
        f.write(b'\x00' * bin_size_padded) 
        f.write(relocation_table)

    with open(args.output_bin, "r+b") as f:
        output_data = f.read()
        data_to_crc = output_data[HEADER_SIZE + description_fixed_size:]
        print(f"[*] Calculating CRC32 for data starting from offset {HEADER_SIZE + description_fixed_size}; len = {len(data_to_crc)}...")
        crc32 = zlib.crc32(data_to_crc) & 0xFFFFFFFF
        print(f"[*] Calculated CRC32: {crc32:#010x}")
        f.seek(12)
        f.write(struct.pack("<I", crc32))

    print(f"Header + description + relocation table added. Found {relocation_count} relocations. Total size: {HEADER_SIZE + description_fixed_size + fixed_bin_size + len(relocation_table)} bytes. Padded size: {bin_size_padded} bytes.")
    print(f"Output binary written to: {args.output_bin}")
    print(f"Entry point offset: {entry_offset} bytes (after header and description)")

if __name__ == "__main__":
    main()
