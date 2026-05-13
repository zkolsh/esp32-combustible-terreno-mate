#ifndef CONFIG_H
#define CONFIG_H

#include <atomic>
#include <cstdint>
#include <WiFi.h>
#include <MFRC522.h>
#include <DS1302.h>

#define SS_PIN 5 	/* Pin del ESP32 conectado a SS del lector RFID */
#define RST_PIN 22 	/* Pin del ESP32 conectado a RST del lector RFID */
#define LED_PIN 2 	/* Pin del ESP32 conectado al LED indicador */
#define SENSOR  27 	/* Pin del sensor de flujo de líquido */
#define SWITCH_PIN 14 	/* Pin del Switch */
#define MOTOR_PIN 13 	/* Pin del Motor */

#define SETBIT(var, nbit) ((var) |= (1UL << (nbit)))
#define CLRBIT(var, nbit) ((var) &= ~(1UL << (nbit)))
#define BITAT(var, nbit) ((var) & (1 << (nbit)))

#define WIFI_SSID "EINGE"
#define WIFI_PASSWORD "EINGE3EINGE"

#define SERVER_HOST "www.eingenet.com.ar"
#define SERVER_PORT 1980
#define RTU_IMEI "145656998240506"
// const char* imei2 = "149293600334943";

/* [WAITING_ACK_TIMEOUT]: ms */
#define WAITING_ACK_TIMEOUT 5000

#define TASK_STACK_SIZE 8192 /* words */
#define MAX_REGISTRO_CARGAS 384

extern bool cardPresent;
extern const unsigned long cardTimeout; // Tiempo de espera para tarjeta
extern byte lastCardID[4];

extern unsigned long lastCardTime;
extern unsigned long cardDetectedSeconds;

extern unsigned long lastMillis;
extern long seconds;
extern long noEsperarACK;
extern std::atomic<bool> pendingMcast;

extern int RFIDDetectada; 
extern unsigned long io;
extern unsigned long newIO;

extern MFRC522 mfrc522;  //Instancia del lector de tarjetas
extern WiFiClient client;
extern uint16_t totalPulses;
extern uint16_t lostPulses;
extern int32_t idTanque;
extern uint32_t tiempoTotalCarga;

#define K_CE_PIN 26 /* Chip Enable */
#define K_IO_PIN 25 /* Input/Output */
#define K_SCLK_PIN 33 /* Serial Clock */
extern DS1302 rtc;  // Instancia del RTC DS1302

#define Q_MAX_SIZE 256
extern char MsgBox_out[Q_MAX_SIZE];

#define FDL "\r\n"
#define KEY9600 "\x7e\r\n"
#define KEY300 "/?!\r\n"
#define Respuesta_ACK_Actaris "\x06""\xB2""\x35""\xB2""\x8D""\x0A"

#define NUMBER_OF_DIGITS 12
extern char buffLTOA[NUMBER_OF_DIGITS];

extern char dataJson[512];
extern char mcastJson[256];

extern char cp[29 + 1];

//Estructura de la fecha y hora
struct DateTime_t
{            
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hh;
	uint8_t mm;
	uint8_t ss;
	uint8_t weekDay;
};

struct Carga {
	unsigned long io;
	uint32_t tiempoCarga;
	int32_t idTanque;
	int32_t gasoilAisgnado;
	int32_t gasoilNoAisgnado;
	int32_t cargaPromedio;
	uint16_t totalPulses;
	uint16_t lostPulses;

	bool IsMCast() const {
		return lostPulses == UINT16_MAX && totalPulses == UINT16_MAX;
	}
};

#endif
