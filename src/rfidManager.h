#ifndef RFIDMANAGER_H
#define RFIDMANAGER_H

#include <MFRC522.h>
#include <vector>
#include <ArduinoJson.h>

// Tamaño máximo de la lista de tarjetas
#define MAX_CARDS 30
#define CARD_LIST_FILE "/rfid_list.json"

// Estructura para almacenar las ID de las tarjetas y su número asignado
struct RFIDCard {
	byte id[4];
	int assignedNumber;
};

// Lista de tarjetas RFID detectadas
inline std::vector<RFIDCard> cardList;

int findCardInList(byte* cardID); // Función para verificar si una tarjeta ya está en la lista
void saveCardList(); // Función para guardar la lista de tarjetas en SPIFFS
void addCardToList(byte* cardID); // Función para añadir una nueva tarjeta a la lista
void loadCardList(); // Función para cargar la lista de tarjetas desde SPIFFS
void handleRFID(); // Función para manejar la lectura de tarjetas RFID
void printCardList();
void handleCardDetection();
void handleCardRemoval();

#endif
