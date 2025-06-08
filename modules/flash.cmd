rem st-flash write test.bin 0x08060000

python create_flash_bundle.py -a 0x08060000 -m test.bin -m test.bin 
