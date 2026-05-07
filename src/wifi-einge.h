#ifndef WIFI_EINGE_H
#define WIFI_EINGE_H
#include <WiFi.h>
#include "config.h"

// Función de Conexión WIFI
void connectToWiFi(const char* ssid, const char* password) {
	Serial.print("Connecting to ");
	Serial.println(ssid);

	WiFi.begin(ssid, password);

	Serial.println("");
	Serial.println("WiFi connected");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	delay(5000);
	Serial.print("connecting to ");
	Serial.println(host);
}
//Función de reconexión WIFI
void reconnectToWiFi() {
	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("Reconnecting to WiFi...");
		WiFi.disconnect();
		WiFi.begin(ssid, password);

		if (WiFi.status() == WL_CONNECTED) {
			Serial.println("Reconnected to WiFi.");
		} else {
			Serial.println("Failed to reconnect to WiFi.");
		}
	}
}
//Función de Reconexion al Servidor
void reconnectToServer() {
	if (!client.connected()) {
		Serial.println("Reconnecting to server...");
		client.stop();
		if (client.connect(host, port)) {
			Serial.println("Reconnected to server.");
		} else {
			Serial.println("Failed to reconnect to server.");
		}
	}
}

#endif
