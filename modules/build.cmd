
arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -nostdlib -nodefaultlibs -nostartfiles -fno-exceptions -fno-rtti -ffreestanding -c test.cpp -o test.o
arm-none-eabi-ld --emit-relocs -T test.ld -o test.elf test.o
arm-none-eabi-objcopy -O binary -j .text -j .rodata -j .data -j .bss test.elf test_.bin
python create_bin.py --elf test.elf -i test_.bin -o test.bin -d "This is a test module" --verbose

