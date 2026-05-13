#include "gestorTelemetria.h"

#include "config.h"
#include "bufferSPIFFS.h"
#include "tqueue.h"
#include "wifi-einge.h"

#include <WiFi.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <DS1302.h>

#define S_ERROR 0
#define S_ACK 1

enum ST {
	ST_CONNECTING,
	ST_KEEP_ALIVE,
	ST_TUNNEL,
	ST_WAITING_ACK
};

static ST estadoConexion;
static unsigned long startKeepAlivems;

static int packet = 0;
static TQueue<Carga, MAX_REGISTRO_CARGAS>* RegistroCargas = nullptr;

static void actualizarEstado();

#define BUFFER_SIZE 1024
static char buff[BUFFER_SIZE]; //buffer de datos

void taskGestorTelemetria(void* registroCargas_) {
	RegistroCargas = reinterpret_cast<TQueue<Carga, MAX_REGISTRO_CARGAS>*>(registroCargas_);

	rtc.halt(false);          // Asegura que el RTC no está detenido
	rtc.writeProtect(false);  // Desactiva la protección de escritura

	if (!SPIFFS.begin(true)) {
		Serial.println("An error occurred while mounting SPIFFS");
		vTaskDelete(nullptr);
		return;
	}

	//Conexion inicial a WIFI y Servidor
	connectToWiFi(WIFI_SSID, WIFI_PASSWORD);
	estadoConexion = ST_CONNECTING;
	Serial.println(":: taskGestorTelemetria inicializado.");

	while (true) {
		// Serial.println(">>> taskGestorTelemetria: actualizarEstado()");
		actualizarEstado();
		// Serial.println("<<< taskGestorTelemetria: actualizarEstado()");
		vTaskDelay(pdMS_TO_TICKS(70));
	};

	vTaskDelete(nullptr);
};

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
static int sendKeepAlive(const uint32_t packetNumber) {
	Serial.println(">>> sendKeepAlive()");

	// Obtener la hora del RTC
	Time t = rtc.time();
	char currentTime[37];
	snprintf(currentTime, sizeof(currentTime),
		"%04d-%02d-%02dT%02d:%02d:%02d.000-03:00",
		t.yr, t.mon, t.date, t.hr, t.min, t.sec);

	// Enviar el primer jdata (IMEI 1)
	snprintf(dataJson, sizeof(dataJson), "jdata '{\
			\"v\":\"8\",\
			\"hs\":\"esp32\",\
			\"im\":\"%s\",\
			\"id\":\"1\",\
			\"DT\":\"%s\",\
			\"rh\":\"0\",\
			\"rh1\":\"0\",\
			\"rh2\":\"0\",\
			\"rh3\":\"0\",\
			\"f\":\"0\",\
			\"io\":\"%lu\",\
			\"t0\":\"60\",\
			\"an\":[{\
				\"a\":[%d,%d,%d]},\
				{\"a\":[%d,%d,%d]},\
				{\"a\":[%d,%d,%d]},\
				{\"a\":[%d,%d,%d]}],\
				\"pq\":\"%u\"\
		}'", RTU_IMEI, currentTime, io,
		idTanque, idTanque, idTanque,
		totalPulses, totalPulses, totalPulses,
		lostPulses, lostPulses, lostPulses,
		tiempoTotalCarga, tiempoTotalCarga, tiempoTotalCarga,
		packetNumber);
	Serial.println(dataJson);
	client.print(dataJson);
	client.print(FDL);
	client.flush();

	Serial.println("<<< sendKeepAlive()");
	return S_ACK;
}

static bool enviarRegistro(const uint32_t packetNumber) {
	if (!RegistroCargas) return false;
	if (RegistroCargas->isEmpty()) return false;

	Time t = rtc.time();
	char currentTime[37];
	snprintf(currentTime, sizeof(currentTime),
		"%04d-%02d-%02dT%02d:%02d:%02d.000-03:00",
		t.yr, t.mon, t.date, t.hr, t.min, t.sec);

	Carga c = RegistroCargas->pop();
	if (c.tiempoCarga > 0 && !c.IsMCast()) {
		snprintf(dataJson, sizeof(dataJson),
			R"(jdata '{"v":"8","hs":"esp32","im":"%s","id":"1","DT":"%s",)"
			R"("rh":"0","rh1":"0","rh2":"0","rh3":"0","f":"0","io":"%lu",)"
			R"("t0":"60","an";[{"a":[%d,%d,%d]},{"a":[%d,%d,%d]},)"
			R"({"a":[%d,%d,%d]},{"a":[%d,%d,%d]}],"pq":"%u"}')",
			RTU_IMEI, currentTime, c.io,
			c.idTanque, c.idTanque, c.idTanque,
			c.totalPulses, c.totalPulses, c.totalPulses,
			c.lostPulses, c.lostPulses, c.lostPulses,
			c.tiempoCarga, c.tiempoCarga, c.tiempoCarga,
			packetNumber);

		Serial.println(dataJson);
		client.print(dataJson);
		client.print(FDL);
		client.flush();
		return true;
	} else {
	 	snprintf(mcastJson, sizeof(mcastJson), "mcast <4>%s{%d}{%lu}{%lu}" FDL, RTU_IMEI, 1, c.io, c.io);
	 	Serial.println(mcastJson);
	 	client.print(mcastJson);
	 	client.print(FDL);
	 	client.flush();
	 	return false;
 	};
};

static void actualizarEstado() {
	//Maquina de estados para funcionalidades
	switch (estadoConexion) {
	case ST_CONNECTING: /* ok o error */
		Serial.println(">>> ST_CONNECTING");
		connectToWiFi(WIFI_SSID, WIFI_PASSWORD);

		if (!client.connect(SERVER_HOST, SERVER_PORT)) {
			Serial.println("Conexion fallida, continuando con el programa");
			estadoConexion = ST_KEEP_ALIVE;
		} else {
			Serial.println("OK>>To ST_KEEP_ALIVE...");
			//periodicSend();
			client.setNoDelay(true);
			seconds = 60;
			estadoConexion = ST_KEEP_ALIVE;
		}

		Serial.println("<<< ST_CONNECTING");
		break;

	case ST_KEEP_ALIVE: /* ack, error, o tunel */
		if (seconds < 60) break;

		Serial.println(">>> ST_KEEP_ALIVE");
		seconds = 0;

		packet = (packet + 1) % 10;
		if (sendKeepAlive(packet) == S_ACK) {
			startKeepAlivems = millis();
			if (noEsperarACK != 0) {
				Serial.println("OK, ignoring next ACK...");
				noEsperarACK = 0;
			} else {
				Serial.println("OK, changing state to ST_WAITING_ACK...");
				estadoConexion = ST_WAITING_ACK;
			}
		} else {
			Serial.println("ERROR -> Back to ST_CONNECTING...");
			estadoConexion = ST_CONNECTING;
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
			        idTanque = 0;
			}

			estadoConexion = ST_CONNECTING;
	     } else if (size == 0) {
			Serial.println("ESPERANDO");
	     } else if (size > 0) {
			size = (size >= BUFFER_SIZE ? BUFFER_SIZE : size);
			client.read((uint8_t*)buff, size);
			
			// Procesar la respuesta JSON y actualizar RTC
			JsonDocument doc{};
			DeserializationError error = deserializeJson(doc, buff);

			if (!error) {
				const char* dateStr = doc["DT"];
				DateTime_t dt;

				if (dateStr && parseISO8601StrToDateTime(&dt, dateStr) == 0) {
					// Actualizar RTC con la nueva fecha/hora
					rtc.time(Time(
						dt.year,
						dt.month,
						dt.day,
						dt.hh,
						dt.mm,
						dt.ss,
						Time::kSunday /* FIXME */
					));

					Serial.println("RTC actualizado con la fecha del servidor");
				}
			}

			Serial.printf("RECIBIDO:%s", buff);
			Serial.println("ACK recibido");
			// sendSavedData();
			
			estadoConexion = ST_KEEP_ALIVE;
			break;
		}

		delay(500);
		Serial.println("<<< ST_WAITING_ACK");
		break;
	};

	default:
		Serial.printf("WARNING: ST connection state is unmanaged (%i)\n", (int)estadoConexion);
	}

	if (client.available()) {
		if (enviarRegistro(packet)) {
			packet = (packet + 1) % 10;
		};
	} else {
	};
};
