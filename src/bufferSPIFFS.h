#ifndef BUFFERSPIFFS_H
#define BUFFERSPIFFS_H

#include <SPIFFS.h>
#include "config.h"

void saveToSPIFFS(const char *dataJson);

void sendSavedData(); 

// Llamar a esta función periódicamente en el loop()
void periodicSend();

#endif
