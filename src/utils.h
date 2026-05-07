#ifndef UTILS_H
#define UTILS_H

#define S_ERROR 0
#define S_ACK 1

#include <DS1302.h>
#include "config.h"

extern void saveToSPIFFS(const char* dataJson);
extern void sendSavedData();

void initSerial() {
	Serial.begin(115200, SERIAL_8N1);
}

void IRAM_ATTR pulseCounter() {
	pulseCount++;
}

bool isSwitchActivated() {
	return digitalRead(SWITCH_PIN) == LOW;
}

int RFIDDetectadaFunc() {
	return RFIDDetectada;
}

int initRTU() {
	return 0;
}
//Funcion de creación y envio del KeepALive
int sendKeepAlive(const uint32_t packetNumber) {
	Serial.println(">>> sendKeepAlive()");
	uint32_t contador = totalPulses;
	uint32_t perdidas = lostPulses;

	// Obtener la hora del RTC
	Time t = rtc.time();
	char currentTime[37];
	snprintf(currentTime, sizeof(currentTime), "%04d-%02d-%02dT%02d:%02d:%02d.000-03:00", t.yr, t.mon, t.date, t.hr, t.min, t.sec);

	// Enviar el primer jdata (IMEI 1)
	snprintf(dataJson, sizeof(dataJson),
			"jdata '{\"v\":\"8\",\"hs\":\"esp32\",\"im\":\"%s\",\"id\":\"1\",\"DT\":\"%s\",\"rh\":\"0\",\"rh1\":\"0\",\"rh2\":\"0\",\"rh3\":\"0\",\"f\":\"0\",\"io\":\"%lu\",\"t0\":\"60\",\"an\":[{\"a\":[%d,%d,%d]},{\"a\":[%d,%d,%d]},{\"a\":[%d,%d,%d]},{\"a\":[%d,%d,%d]}],\"pq\":\"%u\"}'",
			imei, currentTime, io, tanque, tanque, tanque, contador, contador, contador, perdidas, perdidas, perdidas, tiempoTotalCarga, tiempoTotalCarga, tiempoTotalCarga, packetNumber);
	Serial.println(dataJson);
	client.print(dataJson);
	client.print(FDL);
	client.flush();

	Serial.println("<<< sendKeepAlive()");
	return S_ACK;
}


void sendMcast() {
	Serial.println(">>> sendMcast()");
	snprintf(mcastJson, sizeof(mcastJson), "mcast <4>%s{%d}{%lu}{%lu}" FDL, imei, 1, io, io);
	Serial.println(mcastJson);
	client.print(mcastJson);
	client.print(FDL);
	delay(100);
	client.flush();
	Serial.println("<<< sendMcast()");
}

#endif
