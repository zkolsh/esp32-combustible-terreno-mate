#define FILE_NAME "/jdata.txt"
#include "bufferSPIFFS.h"

#include <SPIFFS.h>
#include "config.h"

File getNewSPIFFS() {
	File f = SPIFFS.open(FILE_NAME, "w");
	if (!f) {
		Serial.println("Failed to open file for writing");
	};

	return f;
};

void saveToSPIFFS(File file, size_t length, const uint8_t* data) {
	file.seek(file.size());
	if (!file.write(data, length)) {
		Serial.println("!!! Fallo de escritura en SPIFFS");
	}
}

bool hasDataInSPIFFS() {
	File f = SPIFFS.open(FILE_NAME, "w");
	if (!f) {
		return false;
	};

	f.seek(0, SeekEnd);
	size_t len = f.position();
	return len != 0;
};

File openSPIFFS() {
	File f = SPIFFS.open(FILE_NAME, "r");
	if (!f) {
		Serial.println("Failed to open file for writing");
	};

	return f;
};

// void sendSavedData() {
// 	Serial.println(">>> sendSavedData()");
// 	File file = SPIFFS.open(FILE_NAME, "r");
// 	if (!file) {
// 		Serial.println("Failed to open file for reading");
// 		return;
// 	}
//
// 	String data = file.readStringUntil('\n');
// 	file.close();
//
// 	if (data.length() > 0 && WiFi.status() == WL_CONNECTED && client.connected()) {
// 		client.print(data);
// 		client.print(FDL);
// 		client.flush();
//
// 		Serial.print("Data sent to server: ");
// 		Serial.println(data);
//
// 		// Ahora, para borrar la línea enviada, se crea un archivo temporal
// 		File tempFile = SPIFFS.open("/temp.txt", "w");
// 		if (!tempFile) {
// 			Serial.println("Failed to open temp file for writing");
// 			return;
// 		}
//
// 		tempFile.close();
//
// 		file = SPIFFS.open(FILE_NAME, "r");
// 		while (file.available()) {
// 			String line = file.readStringUntil('\n');
// 			if (line != data) {
// 				tempFile.println(line);
// 			}
// 		}
// 		file.close();
//
// 		// Borrar el archivo original y renombrar el archivo temporal
// 		SPIFFS.remove(FILE_NAME);
// 		SPIFFS.rename("/temp.txt", FILE_NAME);
//
// 		Serial.println("Line sent and removed from SPIFFS");
// 	} else {
// 		Serial.println("No data sent. Either no data to send or connection issue.");
// 	}
// 	Serial.println("<<< sendSavedData()");
// }
