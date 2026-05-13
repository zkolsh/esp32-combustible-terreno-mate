#include "gestorCarga.h"

#include "config.h"
#include "tqueue.h"
#include "rfidManager.h"
#include <Arduino.h>
#include <cstdint>

// Estados para la medición de carga
enum class EstadoCarga {
	SinTarjeta,
	TarjetaDetectada,
	Tolerancia,
	ContandoCarga
};

static EstadoCarga estadoActual = EstadoCarga::SinTarjeta;
static TQueue<Carga, MAX_REGISTRO_CARGAS>* RegistroCargas = nullptr;

static inline const char* EstadoCargaStr(const EstadoCarga e) noexcept {
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
static unsigned long tiempoInicioTolerancia = 0;
static unsigned long tiempoInicioCarga = 0;
static unsigned long tiempoTolerancia = 1000;  // serían 1 segundos

static long currentMillis = 0;
static byte pulse1Sec = 0;
static float flowRate = 0;

static void actualizarEstado();

volatile byte pulseCount;   
void IRAM_ATTR pulseCounter() {
	pulseCount++;
}

static inline bool isSwitchActivated() {
	return digitalRead(SWITCH_PIN) == LOW;
}

static inline int RFIDDetectadaFunc() noexcept {
	return RFIDDetectada;
}

static inline void registrarCarga(unsigned long io_) {
	Carga c{};
	c.io = io_;
	c.tiempoCarga = tiempoTotalCarga;
	c.gasoilAisgnado = 0;
	c.gasoilNoAisgnado = 0;
	c.idTanque = idTanque;
	c.cargaPromedio = 0;
	c.totalPulses = totalPulses;
	c.lostPulses = lostPulses;

	if (RegistroCargas) {
		RegistroCargas->push(std::move(c));
	};
}

static inline void registrarMcast(unsigned long io_) {
	Carga c = {0};
	c.io = io_;
	c.totalPulses = c.lostPulses = UINT16_MAX;

	if (RegistroCargas) {
		RegistroCargas->push(std::move(c));
	};
}

void taskGestorCarga(void* registroCargas_) {
	RegistroCargas = reinterpret_cast<TQueue<Carga, MAX_REGISTRO_CARGAS>*>(registroCargas_);

	SPI.begin();
	mfrc522.PCD_Init();
	attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);

	loadCardList();

	Serial.println(":: taskGestorCarga inicializado.");
	while (true) {
		actualizarEstado();
		//Serial.println("<<< taskGestorCarga: actualizarEstado()");
		vTaskDelay(pdMS_TO_TICKS(7));
	};

	vTaskDelete(nullptr);
};

static void actualizarEstado() {
	newIO = io;
	
	if (mfrc522.PICC_IsNewCardPresent() || mfrc522.PICC_ReadCardSerial()) {
		handleCardDetection();
		SETBIT(newIO, 10);
	} else {
		cardPresent = false;
		CLRBIT(newIO, 10);
	};

	//Funcion para el estado del Switch
	const bool switchActive = isSwitchActivated();
	if (switchActive) {
		SETBIT(newIO, 9);
	} else {
		CLRBIT(newIO, 9);
	}

	currentMillis = millis();
	if (cardPresent || switchActive) {
		SETBIT(newIO, 0);
		digitalWrite(MOTOR_PIN, HIGH);
		pulse1Sec = pulseCount;
		totalPulses += pulseCount;
		pulseCount = 0;
	} else {
		CLRBIT(newIO, 0);
		digitalWrite(MOTOR_PIN, LOW);

		// Acumula los pulsos como pérdida cuando no hay tarjeta ni interruptor activado
		lostPulses += pulseCount;
		pulseCount = 0;
	};


	if (cardPresent || flowRate > 0 || switchActive) {
		digitalWrite(LED_PIN, HIGH);
	} else {
		digitalWrite(LED_PIN, LOW);
	}

	if ((currentMillis - lastMillis) > 1000) {
		const unsigned long dt = (currentMillis - lastMillis) / 1000;
		seconds += dt;
		lastMillis += 1000 * dt;
	}

	// Nexo con gestorTelemetria
	if (io != newIO) {
		Serial.println("IO cambió, guardando captura del estado");
		Serial.println(io);
		Serial.println(newIO);

		registrarMcast(io = newIO);
		if (RegistroCargas) {
			seconds = 60;
			noEsperarACK = 1;
		};
	}

	// Máquina de estados de tiempo de carga
	// const auto e0 = EstadoCargaStr(estadoActual);
	// Serial.printf(">>> %s\n", e0);
	switch (estadoActual) {
		case EstadoCarga::SinTarjeta:
			if (cardPresent) {
				estadoActual = EstadoCarga::TarjetaDetectada;
				registrarMcast(io);
			}
			break;

		case EstadoCarga::TarjetaDetectada:
			estadoActual = EstadoCarga::Tolerancia;
			tiempoInicioTolerancia = tiempoInicioCarga = millis();
			break;

		case EstadoCarga::Tolerancia:
			if (millis() - tiempoInicioTolerancia < tiempoTolerancia) {
				break;
			};

			if (cardPresent) {
				//cuando termina la tolerancia sigue contando tiempo de carga
				estadoActual = EstadoCarga::ContandoCarga;
				tiempoInicioCarga = millis();
			} else {
				handleCardRemoval();
				estadoActual = EstadoCarga::SinTarjeta;
				registrarMcast(io);
			};

			break;

		case EstadoCarga::ContandoCarga:
			if (!cardPresent) {
				estadoActual = EstadoCarga::SinTarjeta;  // Para conteo de carga si la tarjeta se saca y guarda el valor del tiempo total de carga
				tiempoTotalCarga = (millis() - tiempoInicioCarga) / 1000;
				registrarMcast(io);
				registrarCarga(io);
			}
			break;
	}
	// Serial.printf("<<< %s\n", e0);
};
