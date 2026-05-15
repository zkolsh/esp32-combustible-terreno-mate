#include "src/config.h"
#include "src/tqueue.h"
#include "src/gestorCarga.h"
#include "src/gestorTelemetria.h"

#include <Arduino.h>
#include <DS1302.h>
#include <freertos/task.h>

static TaskHandle_t carga = nullptr;
static TaskHandle_t telemetria = nullptr;

TQueue<Carga, MAX_REGISTRO_CARGAS> registroCargas{};

inline void initSerial() {
	Serial.begin(115200, SERIAL_8N1);
};

void setup() {
  initSerial();

  // Configuración de Pines
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(SENSOR, INPUT_PULLUP);

	rtc.halt(false);          // Asegura que el RTC no está detenido
	rtc.writeProtect(false);  // Desactiva la protección de escritura

  // Creacion de tareas
  BaseType_t xr;

  xr = xTaskCreate(taskGestorCarga, "GestorCarga", TASK_STACK_SIZE, (void*)&registroCargas, 2, &carga);
  if (!xr) {
    Serial.println("!!! No se pudo inicializar GestorCarga.");
  };

  xr = xTaskCreate(taskGestorTelemetria, "GestorTelemetria", TASK_STACK_SIZE, (void*)&registroCargas, 1, &telemetria);
  if (!xr) {
    Serial.println("!!! No se pudo inicializar GestorTelemetria.");
  };
}

void loop() {
}
