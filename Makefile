.phony: build upload

build:
	make -f ${HOME}/Lab/makeEspArduino/makeEspArduino.mk ESP_ROOT=${HOME}/Lab/esp8266 CHIP=esp8266 BOARD=d1_mini FLASH_DEF=4M1M all

# build and upload
upload:
	make -f ${HOME}/Lab/makeEspArduino/makeEspArduino.mk ESP_ROOT=${HOME}/Lab/esp8266 CHIP=esp8266 BOARD=d1_mini FLASH_DEF=4M1M ESP_ADDR=doorbell ESP_PWD=${OTA} ota

#~/Lab/esp8266/tools/espota.py -i doorbell -p 8266 -a ${OTA} -r -f /tmp/mkESP/Doorbell_d1_mini/Doorbell.bin
