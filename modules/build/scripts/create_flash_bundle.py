import argparse
import os
import struct
import random
import string
import subprocess

HEADER_SIGNATURE = int.from_bytes(b'AIKB', byteorder='little')  # 'AIKa Bundle'
HEADER_SIZE = 32  # 32 header

def get_file_size(file_path):
    return os.path.getsize(file_path)

def align_address(address, alignment=0x8):
    return (address + alignment - 1) & ~(alignment - 1)

def generate_random_name(prefix="bundle", ext="bin"):
    rand_str = ''.join(random.choices(string.ascii_lowercase + string.digits, k=4))
    return f"{prefix}_{rand_str}.{ext}"

def make_bundle(output_path, modules):
    
    with open(output_path, 'wb') as out_file:

        modules_count = len(modules)

        header_pos = out_file.tell()
        out_file.write(b'\x00' * HEADER_SIZE)  # 8 x uint32
        out_file.write(b'\x00' * 4 * modules_count)

        module_offsets = []
        current_offset = out_file.tell()

        for module in modules:
            if not os.path.isfile(module):
                raise FileNotFoundError(f"Module file not found: {module}")

            with open(module, 'rb') as f:
                data = f.read()

            padding = align_address(len(data)) - len(data)
            padded_data = data + b'\x00' * padding

            module_offsets.append(current_offset)

            out_file.write(padded_data)
            current_offset += len(padded_data)

        out_file.seek(header_pos)
        out_file.write(struct.pack('<I', HEADER_SIGNATURE))
        out_file.write(struct.pack('<I', modules_count))
        out_file.write(struct.pack('<I', 0))  # reserved
        out_file.write(struct.pack('<I', 0))  # reserved
        out_file.write(struct.pack('<I', 0))  # reserved
        out_file.write(struct.pack('<I', 0))  # reserved
        out_file.write(struct.pack('<I', 0))  # reserved
        out_file.write(struct.pack('<I', 0))  # reserved

        print(f"[+] Bundle created: {output_path}")
        print(f"[*] Module offsets:")
        for i, offset in enumerate(module_offsets):
            out_file.seek(HEADER_SIZE + i * 4) # 16 bytes for header + 4 bytes per module offset
            out_file.write(struct.pack('<I', offset))
            print(f"     Module {i}: 0x{offset:08X}")

def main():
    parser = argparse.ArgumentParser(description="Flash multiple modules to STM32 using st-flash")
    parser.add_argument('-a', '--address', required=False, help='Start address (e.g., 0x08060000) (optional) if not specified, will not flash)')
    parser.add_argument('-o', '--output', default=None, help='Output bundle file (optional)')
    parser.add_argument('-m', '--module', action='append', required=True, help='Binary module file (.bin)')
    parser.add_argument('--align', type=int, default=0x8, help='Align address to this value (default: 0x8)')
    args = parser.parse_args()

    if args.address:
        try:
            current_address = int(args.address, 16)
        except ValueError:
            print(f"[!] Invalid address: {args.address}")
            exit(1)

    print(f"[*] Starting flashing at address: 0x{current_address:08X}")
    print(f"[*] Alignment: 0x{args.align:X}")

    output_path = args.output
    if output_path is None:
        output_path = generate_random_name()
        print(f"[+] Output not specified. Generated name: {output_path}")

    make_bundle(output_path, args.module)

    if args.address:
        # call st-flash
        cmd = ['st-flash', 'write', output_path, f'0x{current_address:X}']
        print(" ".join(cmd))
        result = subprocess.run(cmd, check=True)

        if result.returncode != 0:
            print(f"[!] Failed to flash {output_path}")
            exit(1)

if __name__ == '__main__':
    main()
