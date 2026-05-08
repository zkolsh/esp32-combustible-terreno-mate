#include "bufferSPIFFS.h"

#include <SPIFFS.h>
#include "config.h"

void saveToSPIFFS(const char *dataJson) {
	File file = SPIFFS.open(FILE_NAME, "a");
	if (!file) {
		Serial.println("Failed to open file for writing");
		return;
	}

	file.seek(file.size());  // Moverse al final del archivo
	if (file.peek() != '\n') {
		file.println();
	}

	if (file.println(dataJson)) {
		Serial.println("Data saved to SPIFFS");
	} else {
		Serial.println("Write failed");
	}

	file.close();
}

void sendSavedData() {
	Serial.println(">>> sendSavedData()");
	File file = SPIFFS.open(FILE_NAME, "r");
	if (!file) {
		Serial.println("Failed to open file for reading");
		return;
	}

	String data = file.readStringUntil('\n');
	file.close();

	if (data.length() > 0 && WiFi.status() == WL_CONNECTED && client.connected()) {
		client.print(data);
		client.print(FDL);
		client.flush();

		Serial.print("Data sent to server: ");
		Serial.println(data);

		// Ahora, para borrar la línea enviada, se crea un archivo temporal
		File tempFile = SPIFFS.open("/temp.txt", "w");
		if (!tempFile) {
			Serial.println("Failed to open temp file for writing");
			return;
		}

		file = SPIFFS.open(FILE_NAME, "r");
		while (file.available()) {
			String line = file.readStringUntil('\n');
			if (line != data) {
				tempFile.println(line);
			}
		}
		file.close();
		tempFile.close();

		// Borrar el archivo original y renombrar el archivo temporal
		SPIFFS.remove(FILE_NAME);
		SPIFFS.rename("/temp.txt", FILE_NAME);

		Serial.println("Line sent and removed from SPIFFS");
	} else {
		Serial.println("No data sent. Either no data to send or connection issue.");
	}
	Serial.println("<<< sendSavedData()");
}

void periodicSend() {
	Serial.println(">>> periodicSend()");
	sendSavedData();
	Serial.println("<<< periodicSend()");
}
