@echo off
cd build\esp32.esp32.ttgo-lora32
\esptool-v4.4-win64\esptool --chip ESP32 merge_bin -o rdzTrovaLaSonda.bin --flash_mode dio --flash_size 4MB 0x1000 rdzTrovaLaSonda.ino.bootloader.bin 0x8000 rdzTrovaLaSonda.ino.partitions.bin 0x10000 rdzTrovaLaSonda.ino.bin 
cd ..\..