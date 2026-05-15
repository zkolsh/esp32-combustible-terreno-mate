#ifndef BUFFERSPIFFS_H
#define BUFFERSPIFFS_H

#include "config.h"

#include <SPIFFS.h>
#include <cstdint>

File getNewSPIFFS(); /* Con permiso de escritura */
void saveToSPIFFS(File f, size_t length, const uint8_t* data);

bool hasDataInSPIFFS();
File openSPIFFS(); /* Solo lectura */

#endif
