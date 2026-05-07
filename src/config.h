#ifndef CONFIG_H
#define CONFIG_H

#include <WiFi.h>

#define SS_PIN 5      // Pin del ESP32 conectado a SS del lector RFID
#define RST_PIN 22    // Pin del ESP32 conectado a RST del lector RFID
#define LED_PIN 2     // Pin del ESP32 conectado al LED indicador
#define SENSOR  27    // Pin del sensor de flujo de líquido
#define SWITCH_PIN 14 // Pin del Switch
#define MOTOR_PIN 13  //Pin del Motor

#define SETBIT(var, nbit) ((var) |= (1UL << (nbit)))
#define CLRBIT(var, nbit) ((var) &= ~(1UL << (nbit)))
#define BITAT(var, nbit) ((var) & (1 << (nbit)))

#define FILE_NAME "/jdata.txt"

const char* ssid = "EINGE";
const char* password = "EINGE3EINGE";

const char* host = "www.eingenet.com.ar";
const int port = 1980;
const char* imei = "145656998240506";
// const char* imei2 = "149293600334943";

/* [WAITING_ACK_TIMEOUT]: ms */
#define WAITING_ACK_TIMEOUT 5000

bool cardPresent = false;
const unsigned long cardTimeout = 5000; // Tiempo de espera para tarjeta
byte lastCardID[4];

long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
float calibrationFactor = 4.5;
volatile byte pulseCount = 0;   
byte pulse1Sec = 0;
float flowRate = 0.0;
int packet = 0;
unsigned int flowMilliLitres = 0;
unsigned long totalMilliLitres = 0;

inline unsigned long lastCardTime = 0;
inline unsigned long cardDetectedSeconds = 0;

inline unsigned long lastMillis = 0;
inline long seconds=0;
inline long noEsperarACK=0;

int RFIDDetectada=0; 
unsigned long io = 0;
unsigned long newIO = 0;

inline WiFiClient client;
inline int32_t totalPulses;
inline int32_t lostPulses;
inline int32_t tanque;
inline uint32_t tiempoTotalCarga;

#define K_CE_PIN 26 /* Chip Enable */
#define K_IO_PIN 25 /* Input/Output */
#define K_SCLK_PIN 33 /* Serial Clock */
inline DS1302 rtc(K_CE_PIN, K_IO_PIN, K_SCLK_PIN);  // Instancia del RTC DS1302

enum ST {ST_CONNECTING, ST_KEEP_ALIVE, ST_TUNNEL, ST_WAITING_ACK};
inline ST estado;

#define Q_MAX_SIZE 256
char MsgBox_out[Q_MAX_SIZE];

#define FDL "\r\n"
#define KEY9600 "\x7e\r\n"
#define KEY300 "/?!\r\n"
#define Respuesta_ACK_Actaris "\x06""\xB2""\x35""\xB2""\x8D""\x0A"

#define NUMBER_OF_DIGITS 12
char buffLTOA[NUMBER_OF_DIGITS];

char dataJson[512];
char mcastJson[256];

char cp[29 + 1];
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

unsigned int strnlen(const char *str, unsigned int max) {
	const char *s;
	unsigned int n = 0;
	for (s = str; *s; ++s) {
		n++;
		if (n >= max) break;
	}
	return n;
}

char *strstr_raw(char *s1, const char *s2, size_t ventana, size_t max) {
	while (max >= ventana) {
		if (memcmp((const char *) s1, (const char *) s2, ventana) == 0) {
			return s1;
		}
		s1++;
		max--;
	}
	return NULL;
}

int parseISO8601StrToDateTime(struct DateTime_t *dt_ptr, const char *str) {
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

#endif
