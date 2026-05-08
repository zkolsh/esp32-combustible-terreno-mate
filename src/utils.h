#ifndef UTILS_H
#define UTILS_H

#define S_ERROR 0
#define S_ACK 1

#include <DS1302.h>
#include "config.h"

extern void saveToSPIFFS(const char* dataJson);
extern void sendSavedData();

inline void initSerial() {
	Serial.begin(115200, SERIAL_8N1);
};

inline bool isSwitchActivated() {
	return digitalRead(SWITCH_PIN) == LOW;
}

inline int RFIDDetectadaFunc() {
	return RFIDDetectada;
}

inline int initRTU() {
	return 0;
}

inline unsigned int strnlen(const char *str, unsigned int max) {
	const char *s;
	unsigned int n = 0;
	for (s = str; *s; ++s) {
		n++;
		if (n >= max) break;
	}
	return n;
}

inline char *strstr_raw(char *s1, const char *s2, size_t ventana, size_t max) {
	while (max >= ventana) {
		if (memcmp((const char *) s1, (const char *) s2, ventana) == 0) {
			return s1;
		}
		s1++;
		max--;
	}
	return NULL;
}

inline int parseISO8601StrToDateTime(struct DateTime_t *dt_ptr, const char *str) {
	int err;
	strlcpy(cp, str, 29);

	char* ptrYear = strtok(cp, "-");
	char* ptrMonth = strtok(NULL, "-");
	char* ptrDay = strtok(NULL, "T");
	char* ptrHH = strtok(NULL, ":");
	char* ptrMM = strtok(NULL, ":");
	char* ptrSS = strtok(NULL, ".");

	if (dt_ptr != NULL &&
			ptrYear != NULL &&
			ptrMonth != NULL &&
			ptrDay != NULL &&
			ptrHH != NULL &&
			ptrMM != NULL &&
			ptrSS != NULL) {
		dt_ptr->year = atoi(ptrYear);
		dt_ptr->month = atoi(ptrMonth);
		dt_ptr->day = atoi(ptrDay);
		dt_ptr->hh = atoi(ptrHH);
		dt_ptr->mm = atoi(ptrMM);
		dt_ptr->ss = atoi(ptrSS);
		err = 0;
	} else {
		err = 1;
	}

	return err;
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
			RTU_IMEI, currentTime, io, tanque, tanque, tanque, contador, contador, contador, perdidas, perdidas, perdidas, tiempoTotalCarga, tiempoTotalCarga, tiempoTotalCarga, packetNumber);
	Serial.println(dataJson);
	client.print(dataJson);
	client.print(FDL);
	client.flush();

	Serial.println("<<< sendKeepAlive()");
	return S_ACK;
}


void sendMcast() {
	Serial.println(">>> sendMcast()");
	snprintf(mcastJson, sizeof(mcastJson), "mcast <4>%s{%d}{%lu}{%lu}" FDL, RTU_IMEI, 1, io, io);
	Serial.println(mcastJson);
	client.print(mcastJson);
	client.print(FDL);
	delay(100);
	client.flush();
	Serial.println("<<< sendMcast()");
}

#endif
