#include "rfidManager.h"

#include "config.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <vector>

int findCardInList(byte* cardID) {
	for (size_t i = 0; i < cardList.size(); i++) {
		if (memcmp(cardList[i].id, cardID, 4) == 0) {
			return cardList[i].assignedNumber;
		}
	}
	return -1;  // No encontrada
}

void saveCardList() {
	File file = SPIFFS.open(CARD_LIST_FILE, "w");
	if (!file) {
		Serial.println("Error al abrir el archivo para guardar la lista de tarjetas");
		return;
	}

	JsonDocument doc{};
	JsonArray array = doc.to<JsonArray>();

	for (const auto& card : cardList) {
		JsonObject obj = array.add<JsonObject>();
		obj["id"] = String(card.id[0], HEX) + ":" + String(card.id[1], HEX) + ":" + String(card.id[2], HEX) + ":" + String(card.id[3], HEX);
		obj["number"] = card.assignedNumber;
	}

	if (serializeJson(doc, file) == 0) {
		Serial.println("Error al escribir en el archivo");
	}

	file.close();
}

void addCardToList(byte* cardID) {
	if (cardList.size() < MAX_CARDS) {
		RFIDCard newCard;
		memcpy(newCard.id, cardID, 4);
		newCard.assignedNumber = cardList.size() + 1;  // Asignar un número secuencial
		cardList.push_back(newCard);
		saveCardList();  // Guardar la lista después de añadir una tarjeta
	} else {
		Serial.println("Lista de tarjetas llena.");
	}
}

void loadCardList() {
	File file = SPIFFS.open(CARD_LIST_FILE, "r");
	if (!file) {
		Serial.println("No se encontró el archivo de lista de tarjetas");
		return;
	}

	JsonDocument doc{};
	DeserializationError error = deserializeJson(doc, file);
	if (error) {
		Serial.print("Error al leer el archivo de lista de tarjetas: ");
		Serial.println(error.c_str());
		file.close();
		return;
	}

	JsonArray array = doc.as<JsonArray>();
	cardList.clear();  // Limpiar la lista actual

	for (const JsonObject& obj : array) {
		RFIDCard card;
		String idStr = obj["id"];
		sscanf(idStr.c_str(), "%02hhX:%02hhX:%02hhX:%02hhX", &card.id[0], &card.id[1], &card.id[2], &card.id[3]);
		card.assignedNumber = obj["number"];
		cardList.push_back(card);
	}

	file.close();
}

void handleRFID() {
	byte currentCardID[4];
	memcpy(currentCardID, mfrc522.uid.uidByte, mfrc522.uid.size);

	int cardNumber = findCardInList(currentCardID);

	if (cardNumber == -1) {
		addCardToList(currentCardID);
		cardNumber = cardList.back().assignedNumber;
		Serial.print("Nueva tarjeta añadida. Número asignado: ");
		Serial.println(cardNumber);
	} else {
		Serial.print("Tarjeta ya registrada. Número asignado: ");
		Serial.println(cardNumber);
	}

	// Enviar el número asignado en lugar de los pulsos perdidos
	idTanque = cardNumber;
}

void printCardList() {
	Serial.println("Lista de tarjetas RFID:");
	for (size_t i = 0; i < cardList.size(); i++) {
		Serial.print("Tarjeta ");
		Serial.print(i + 1);
		Serial.print(": ");
		for (int j = 0; j < 4; j++) {
			Serial.print(cardList[i].id[j], HEX);
			if (j < 3) Serial.print(":");
		}
		Serial.print(" -> Número asignado: ");
		Serial.println(cardList[i].assignedNumber);
	}
}

void handleCardDetection() {
	//Serial.println(">>> handleCardDetection()");
	lastCardTime = millis();  // Actualiza el tiempo de detección de la tarjeta
	cardPresent = true;
	RFIDDetectada = true;

	byte currentCardID[4];
	for (int i = 0; i < 4; i++) {
		currentCardID[i] = mfrc522.uid.uidByte[i];  // Copia la ID de la tarjeta a un arreglo de bytes
	}

	// Compara la nueva ID con la última ID detectada
	const bool isNewCard = memcmp(currentCardID, lastCardID, sizeof(currentCardID)) != 0;
	if (isNewCard) {
		memcpy(lastCardID, currentCardID, sizeof(currentCardID));
		handleRFID();

		Serial.print("Tarjeta cambiada: ");
		for (int i = 0; i < 4; i++) {
			Serial.print(currentCardID[i], HEX);
		}
		Serial.println();
	}

	mfrc522.PICC_HaltA();
	mfrc522.PCD_StopCrypto1();
	//Serial.println("<<< handleCardDetection()");
}

void handleCardRemoval() {
	cardPresent = false;
	RFIDDetectada = false;
	lastCardTime = 0;
}
