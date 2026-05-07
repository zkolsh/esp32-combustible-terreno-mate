#include <WiFi.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>
#include <SPIFFS.h>
#include <DS1302.h>
#include "src/config.h"
#include "src/bufferSPIFFS.h"
#include "src/wifi-einge.h"
#include "src/utils.h"
#include "src/rfidManager.h"

#define BUFFER_SIZE 1024

// Estados para la medición de carga
enum class EstadoCarga {
  SinTarjeta,
  TarjetaDetectada,
  Tolerancia,
  ContandoCarga
};

EstadoCarga estadoActual = EstadoCarga::SinTarjeta;

const char* EstadoCargaStr(const EstadoCarga e) {
  switch (e) {
    case EstadoCarga::SinTarjeta:
      return "SinTarjeta";
    case EstadoCarga::TarjetaDetectada:
      return "TarjetaDetectada";
    case EstadoCarga::Tolerancia:
      return "Tolerancia";
    case EstadoCarga::ContandoCarga:
      return "ContandoCarga";
    default:
      return "EstadoCarga invalido";
  }
}

// Variables
unsigned long tiempoInicioTolerancia = 0;
unsigned long tiempoInicioCarga = 0;
unsigned long tiempoTolerancia = 3000;  //serían 3 segundos

MFRC522 mfrc522(SS_PIN, RST_PIN);  //Instancia del lector de tarjetas
char buff[BUFFER_SIZE];            //buffer de datos

void setup() {
  initSerial();
  SPI.begin();
  mfrc522.PCD_Init();

  //Configuración de Pines
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(SENSOR, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);

  rtc.halt(false);          // Asegura que el RTC no está detenido
  rtc.writeProtect(false);  // Desactiva la protección de escritura

  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  loadCardList();

  //Conexion inicial a WIFI y Servidor
  connectToWiFi(ssid, password);
  Serial.println("");
  Serial.println("To ST_Connecting...");
  estado = ST_CONNECTING;
}

unsigned long startKeepAlivems;

void loop() {
  bool cardDetected = mfrc522.PICC_IsNewCardPresent();
  bool switchActivated = isSwitchActivated();
  //Funcion para Tarjetas RFI

  if (cardDetected) {
    handleCardDetection();
  } else {
    handleCardRemoval();
  }

  //Tiempo de tarjeta en el lector
  if (cardPresent) {
    unsigned long currentSeconds = (millis() - lastCardTime) / 1000;
    if (currentSeconds != cardDetectedSeconds) {
      cardDetectedSeconds = currentSeconds;
      Serial.print("Segundos desde que se detectó la tarjeta: ");
      Serial.println(cardDetectedSeconds);
    }
  }

  //Funcion para el estado del Switch
  const bool switchActive = isSwitchActivated();
  if (switchActive) {
    SETBIT(newIO, 9);
  } else {
    CLRBIT(newIO, 9);
  }

  // Chequea cambios en la variable IO para mcast
  const bool rfidDetected = RFIDDetectadaFunc();
  if (rfidDetected) {
    SETBIT(newIO, 10);
  } else {
    CLRBIT(newIO, 10);
  }

  if (rfidDetected || switchActive) {
    SETBIT(newIO, 0);
  } else {
    CLRBIT(newIO, 0);
  };

  if (BITAT(io, 0)) {
    digitalWrite(MOTOR_PIN, HIGH);
  } else {
    digitalWrite(MOTOR_PIN, LOW);
  }

  if (io != newIO) {
    Serial.println("IO Cambió, enviando MCAST");
    Serial.println(io);
    Serial.println(newIO);
    io = newIO;
    sendMcast();
    seconds = 60;
    noEsperarACK = 1;
  }

  //Conteo de pulsos y flujo
  currentMillis = millis();
  if (cardPresent || switchActive) {
    pulse1Sec = pulseCount;
    totalPulses += pulseCount;
    pulseCount = 0;
  } else if (!cardDetected) {
    // Acumula los pulsos como pérdida cuando no hay tarjeta ni interruptor activado
    lostPulses += pulseCount;
    pulseCount = 0;
  }

  if (cardPresent || flowRate > 0 || switchActivated) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  if ((currentMillis - lastMillis) > 1000) {
    seconds += (currentMillis - lastMillis) / 1000;
    lastMillis = currentMillis;
  }

  // Máquina de estados de tiempo de carga
  Serial.printf(">>> %s\n", EstadoCargaStr(estadoActual));
  switch (estadoActual) {
    case EstadoCarga::SinTarjeta:
      if (cardDetected) {
        estadoActual = EstadoCarga::TarjetaDetectada;
      }
      break;

    case EstadoCarga::TarjetaDetectada:
      estadoActual = EstadoCarga::Tolerancia;
      tiempoInicioTolerancia = tiempoInicioCarga = millis();
      break;

    case EstadoCarga::Tolerancia:
      if (millis() - tiempoInicioTolerancia >= tiempoTolerancia) {
        //cuando termina la tolerancia sigue contando tiempo de carga
        estadoActual = EstadoCarga::ContandoCarga;
      }
      break;

    case EstadoCarga::ContandoCarga:
      if (!cardPresent) {
        estadoActual = EstadoCarga::SinTarjeta;  // Para conteo de carga si la tarjeta se saca y guarda el valor del tiempo total de carga
        tiempoTotalCarga = (millis() - tiempoInicioCarga) / 1000;
      }
      break;
  }
  Serial.printf("<<< %s\n", EstadoCargaStr(estadoActual));

  //Maquina de estados para funcionalidades
  switch (estado) {
    case ST_CONNECTING:  //ok o error
      Serial.println(">>> ST_CONNECTING");
      connectToWiFi(ssid, password);
      if (!client.connect(host, port)) {
        Serial.println("Conexion fallida, continuando con el programa");
        estado = ST_KEEP_ALIVE;
      } else {
        Serial.println("OK>>To ST_KEEP_ALIVE...");
        //periodicSend();
        client.setNoDelay(true);
        seconds = 60;
        estado = ST_KEEP_ALIVE;
      }

      Serial.println("<<< ST_CONNECTING");
      break;

    case ST_KEEP_ALIVE:     //ack, error, o tunel
      Serial.println(">>> ST_KEEP_ALIVE");
      if (seconds >= 60) {  //TODO
        seconds = 0;

        packet = (packet + 1) % 10;
        if (sendKeepAlive(packet) == S_ACK) {
          
          startKeepAlivems = millis();
          if (noEsperarACK != 0) {
            Serial.println("OK, ignoring next ACK...");
            noEsperarACK = 0;
          } else {
            Serial.println("OK, changing state to ST_WAITING_ACK...");
            estado = ST_WAITING_ACK;
          }
        } else {
          Serial.println("ERROR -> Back to ST_CONNECTING...");
          estado = ST_CONNECTING;
        }
      }

      Serial.println("<<< ST_KEEP_ALIVE");
      break;

    case ST_WAITING_ACK: {
      Serial.println(">>> ST_WAITING_ACK");
      int size = client.available();
      unsigned long ms = millis();
      if (size == 0 && ms - startKeepAlivems >= WAITING_ACK_TIMEOUT) {
        Serial.println("Timeout de conexión, guardando jdata en SPIFF e intentando reconectar...");
        saveToSPIFFS(dataJson);
        reconnectToWiFi();
        reconnectToServer();
        startKeepAlivems = totalPulses = lostPulses = 0;
        if (!cardPresent) {
          tanque = 0;
        }

        estado = ST_CONNECTING;
      } else if (size == 0) {
        Serial.println("ESPERANDO");
      };

      if (size > 0) {
        size = (size >= BUFFER_SIZE ? BUFFER_SIZE : size);
        client.read((uint8_t*)buff, size);
        //size = (size >= BUFFER_SIZE ? BUFFER_SIZE : size);
        //client.read((uint8_t*)buff, size);

        // Procesar la respuesta JSON y actualizar RTC
        JsonDocument doc{};
        DeserializationError error = deserializeJson(doc, buff);

        if (!error) {
          const char* dateStr = doc["DT"];
          if (dateStr) {
            DateTime_t dt;
            if (parseISO8601StrToDateTime(&dt, dateStr) == 0) {
              // Actualizar RTC con la nueva fecha/hora
              rtc.time(Time(
                dt.year,
                dt.month,
                dt.day,
                dt.hh,
                dt.mm,
                dt.ss,
                Time::kSunday  // El día de la semana se puede ajustar si es necesario
                ));
              Serial.println("RTC actualizado con la fecha del servidor");
            }
          }
        }

        // Procesar la respuesta recibida si es necesario
        Serial.printf("RECIBIDO:%s", buff);
        Serial.println("ACK recibido");
        // sendSavedData();
        printCardList();
        startKeepAlivems = totalPulses = lostPulses = 0;
        if (!cardPresent) {
          tanque = 0;
        }

        estado = ST_KEEP_ALIVE;
        break;
      }
    
      delay(500);
      Serial.println("<<< ST_WAITING_ACK");
      break;
    };

    default:
        Serial.printf("WARNING: ST connection state is unmanaged (%i)\n", (int)estado);
  }
}
