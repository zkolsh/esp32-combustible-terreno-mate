#include "config.h"

bool cardPresent = false;
const unsigned long cardTimeout = 5000;
byte lastCardID[4] = {0};

long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
float calibrationFactor = 4.5;
byte pulse1Sec = 0;
float flowRate = 0.0;
int packet = 0;
unsigned int flowMilliLitres = 0;
unsigned long totalMilliLitres = 0;

unsigned long lastCardTime = 0;
unsigned long cardDetectedSeconds = 0;

unsigned long lastMillis = 0;
long seconds=0;
long noEsperarACK=0;

int RFIDDetectada = 0; 
unsigned long io = 0;
unsigned long newIO = 0;

MFRC522 mfrc522(SS_PIN, RST_PIN);  //Instancia del lector de tarjetas
WiFiClient client;
int32_t totalPulses;
int32_t lostPulses;
int32_t tanque;
uint32_t tiempoTotalCarga;

DS1302 rtc(K_CE_PIN, K_IO_PIN, K_SCLK_PIN);  // Instancia del RTC DS1302
ST estado{};
char MsgBox_out[Q_MAX_SIZE] = {0};

char buffLTOA[NUMBER_OF_DIGITS] = {0};
char dataJson[512] = {0};
char mcastJson[256] = {0};

char cp[29 + 1] = {0};
