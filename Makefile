SKETCH := esp32-combustible-terreno-mate.ino
LIBS := "MFRC522" "ArduinoJson" "WiFi" "Networking" "SPI" "SPIFFS" "FS" "arduino-ds1302"
PORT := COM6

.PHONY: all flash monitor

all:
	arduino-cli compile -b esp32:esp32:mhetesp32devkit $(SKETCH)

flash:
	arduino-cli upload -b esp32:esp32:mhetesp32devkit -p $(PORT)

monitor:
	arduino-cli monitor -c baudrate=115200 -p $(PORT)

iterate: all
	make flash
	make monitor

libraries:
	@for lib in $(LIBS); do \
		arduino-cli lib install $$lib; \
	done

clean:
	rm -rf build
