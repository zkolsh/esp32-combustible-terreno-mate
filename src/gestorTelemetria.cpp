#include "gestorTelemetria.h"

#include "config.h"
#include "bufferSPIFFS.h"
#include "tqueue.h"
#include "wifi-einge.h"

#include <WiFi.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <DS1302.h>

struct EstadoTelemetria {
	uint32_t nextKeepAliveMs;
	uint32_t ackDeadlineMs;
	uint32_t lastRxMs;

	int32_t keepAliveID;
	uint8_t keepAliveSeq;
	bool waitingAck;
	bool replayingSPIFFS;
};

static void actualizarEstado(EstadoTelemetria&);

static File spiffs;
static TQueue<Carga, MAX_REGISTRO_CARGAS>* RegistroCargas = nullptr;

static uint8_t rxBuffer[512];
static size_t rxLength = 0;

void taskGestorTelemetria(void* registroCargas_) {
	RegistroCargas = reinterpret_cast<TQueue<Carga, MAX_REGISTRO_CARGAS>*>(registroCargas_);

	if (!SPIFFS.begin(true)) {
		Serial.println("An error occurred while mounting SPIFFS");
		vTaskDelete(nullptr);
		return;
	};

	connectToWiFi(WIFI_SSID, WIFI_PASSWORD);

	EstadoTelemetria e = {0};
	Serial.println(":: taskGestorTelemetria inicializado.");
	while (true) {
		// Serial.println(">>> taskGestorTelemetria: actualizarEstado()");
		actualizarEstado(e);
		// Serial.println("<<< taskGestorTelemetria: actualizarEstado()");
		vTaskDelay(pdMS_TO_TICKS(400));
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

static int sendKeepAlive(const uint32_t packetNumber) {
	Serial.println(">>> sendKeepAlive()");

	// Obtener la hora del RTC
	Time t = rtc.time();
	char currentTime[37];
	snprintf(currentTime, sizeof(currentTime),
		"%04d-%02d-%02dT%02d:%02d:%02d.000-03:00",
		t.yr, t.mon, t.date, t.hr, t.min, t.sec);

	// Enviar el primer jdata (IMEI 1)
	snprintf(dataJson, sizeof(dataJson),
		R"(jdata '{"v":"8","hs":"esp32","im":"%s","id":"1","DT":"%s",)"
		R"("rh":"0","rh1":"0","rh2":"0","rh3":"0","f":"0","io":"%lu",)"
		R"("t0":"60","an":[{"a":[%d,%d,%d]},{"a":[%d,%d,%d]},)"
		R"({"a":[%d,%d,%d]},{"a":[%d,%d,%d]}],"pq":"%u"}')",
		RTU_IMEI, currentTime, io,
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
	return true;
}

static bool enviarCarga(const Carga& c, const uint32_t packetNumber) {
	static size_t id = 0;
	char currentTime[37];
	snprintf(currentTime, sizeof(currentTime),
		"%04d-%02d-%02dT%02d:%02d:%02d.000-03:00",
		c.time.yr, c.time.mon, c.time.date, c.time.hr, c.time.min, c.time.sec);

	id++;
	if (c.tiempoCarga > 0 && !c.IsMCast()) {
		snprintf(dataJson, sizeof(dataJson),
			R"(jdata '{"v":"8","hs":"esp32","im":"%s","id":"%zu","DT":"%s",)"
			R"("rh":"0","rh1":"0","rh2":"0","rh3":"0","f":"0","io":"%lu",)"
			R"("t0":"60","an":[{"a":[%d,%d,%d]},{"a":[%d,%d,%d]},)"
			R"({"a":[%d,%d,%d]},{"a":[%d,%d,%d]}],"pq":"%u"}')",
			RTU_IMEI, id, currentTime, io,
			c.idTanque, c.idTanque, c.idTanque,
			c.gasoilAsignado, c.gasoilAsignado, c.gasoilAsignado,
			c.gasoilNoAsignado, c.gasoilNoAsignado, c.gasoilNoAsignado,
			c.tiempoCarga, c.tiempoCarga, c.tiempoCarga,
			packetNumber);

		Serial.println(dataJson);
		client.print(dataJson);
		client.print(FDL);
		client.flush();
		return true;
	} else {
	 	snprintf(mcastJson, sizeof(mcastJson), "mcast <4>%s{%d}{%lu}{%lu}" FDL, RTU_IMEI, 1, io, io);
	 	Serial.println(mcastJson);
	 	client.print(mcastJson);
	 	client.print(FDL);
	 	client.flush();
	 	return false;
 	};
};

static void actualizarEstado(EstadoTelemetria& estado) {
	const auto now = millis();

	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("[t] Timeout de conexión");
		reconnectToWiFi();
		estado.waitingAck = false;
		return;
	};

	if (estado.waitingAck && now > estado.ackDeadlineMs) {
		Serial.println("[t] ACK timeout");
		client.stop();

		if (RegistroCargas->count() >= MAX_REGISTRO_CARGAS) {
			Serial.println("[t] MAX_REGISTRO_CARGAS excedido, guardando datos en SPIFFS...");
			File f = getNewSPIFFS();
			while (!RegistroCargas->isEmpty()) {
				Carga c = RegistroCargas->pop();
				saveToSPIFFS(f, sizeof(Carga), reinterpret_cast<uint8_t*>(&c));
			};
			f.close();
		};

		reconnectToServer();

		estado.nextKeepAliveMs = now + (60 * 1000);
		estado.keepAliveSeq = (estado.keepAliveSeq + 1) % 10;
		if (sendKeepAlive(estado.keepAliveSeq)) {
			estado.waitingAck = true;
			estado.ackDeadlineMs = now + WAITING_ACK_TIMEOUT;
		};

		if (client.connected() && hasDataInSPIFFS()) {
			Serial.println("[t] Conexión recuperada, reproduciendo SPIFFS...");
			spiffs = openSPIFFS();
			estado.replayingSPIFFS = true;
		};

		return;
	};

	while (client.available() > 0) {
		int c = client.read();
		if (c < 0) break;
		estado.lastRxMs = now;

		// Un ACK es un JSON finalizado por un salto de línea
		if (c != '\n') {
			if (rxLength + 1 < sizeof(rxBuffer)) {
				rxBuffer[rxLength++] = c;
			} else {
				rxLength = 0;
			};

			continue;
		};

		/* c == '\n' */
		rxBuffer[rxLength] = 0;
		rxLength = 0;
		Serial.printf("[t] Recibida respuesta del servidor: %s" FDL, rxBuffer);

		JsonDocument doc;
		if (deserializeJson(doc, rxBuffer) != DeserializationError::Ok) {
			continue;
		};

		if (doc["acknol"]) {
			estado.waitingAck = false;
			Serial.println("ACK recibido");
		} else continue;

		const char* dateStr = doc["DT"];
		DateTime_t dt;
		if (dateStr && parseISO8601StrToDateTime(&dt, dateStr) == 0) {
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
		};
	};

	if (estado.waitingAck) {
		return;
	};

	if (now > estado.nextKeepAliveMs) {
		Serial.println("[t] Enviando KeepAlive...");
		estado.keepAliveSeq = (estado.keepAliveSeq + 1) % 10;
		if (sendKeepAlive(estado.keepAliveSeq)) {
			estado.waitingAck = true;
			estado.ackDeadlineMs = now + WAITING_ACK_TIMEOUT;
		};

		estado.nextKeepAliveMs = now + (60 * 1000);
		return;
	};
	
	if (estado.replayingSPIFFS) {
		Serial.println("[t] Enviando SPIFFS...");
		Carga c;
		if (spiffs.read(reinterpret_cast<uint8_t*>(&c), sizeof(Carga)) == sizeof(Carga)) {
			if (estado.keepAliveID != c.idTanque) {
				estado.keepAliveID = c.idTanque;
				estado.keepAliveSeq = 0;
			};

			estado.keepAliveSeq = (estado.keepAliveSeq + 1) % 10;
			if (enviarCarga(c, estado.keepAliveSeq)) {
				estado.waitingAck = true;
				estado.ackDeadlineMs = now + WAITING_ACK_TIMEOUT;
			};
		} else {
			Serial.println("[t] Final de datos en SPIFFS.");
			spiffs.close();
			estado.replayingSPIFFS = false;
		};
		return;
	};

	if (!RegistroCargas) return;

	if (!RegistroCargas->isEmpty()) {
		Serial.println("[t] Enviando registro...");
		Carga c = RegistroCargas->pop();
		if (enviarCarga(c, estado.keepAliveSeq)) {
			estado.keepAliveSeq = (estado.keepAliveSeq + 1) % 10;
			if (!c.IsMCast()) {
				estado.waitingAck = true;
				estado.ackDeadlineMs = now + WAITING_ACK_TIMEOUT;
			};
		};
	};
};
