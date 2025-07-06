rem st-flash write test.bin 0x08060000

rem python create_flash_bundle.py -a 0x08060000 -m test.bin -m test.bin 
python create_flash_bundle.py -a 0x08060000 -m test.bin -m test.bin 
