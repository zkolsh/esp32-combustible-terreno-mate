SKETCH := esp32-combustible-terreno-mate.ino
LIBS := "MFRC522" "ArduinoJson" "WiFi" "Networking" "SPI" "SPIFFS" "FS" "arduino-ds1302"

all:
	arduino-cli compile -b esp32:esp32:mhetesp32devkit $(SKETCH)

libraries:
	@for lib in $(LIBS); do \
		arduino-cli lib install $$lib; \
	done

clean:
	rm -rf build
