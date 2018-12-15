notes from platformio flash log. Maybe useful for manual flashing

setting flash mode from qio to qio
setting flash frequency from 40 to 40
setting flash size from 512K to 4M
added segment #0 to binimage for address 0x40100000 with size 0x00007A80
added section .text at 0x40100000 size 0x00007A80
set bimage entry to 0x40100004
added segment #1 to binimage for address 0x3FFE8000 with size 0x00000850
added section .data at 0x3FFE8000 size 0x0000084E with padding 0x00000002
set bimage entry to 0x40100004
added segment #2 to binimage for address 0x3FFE8850 with size 0x000001F8
added section .rodata at 0x3FFE8850 size 0x000001F8
set bimage entry to 0x40100004

command from other project, for refernce
./esptool.py --port /dev/ttyUSB0 write_flash 0x3fc000 $SDK_PATH/bin/esp_init_data_default.bin 0x00000 $BIN_PATH/eagle.flash.bin 0x20000 $BIN_PATH/eagle.irom0text.bin 

@TODO
prepare command for flashing without platformio or visual code