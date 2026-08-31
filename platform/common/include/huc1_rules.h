#ifndef HUC1_RULES_H
#define HUC1_RULES_H

#include <stdint.h>

namespace huc1 {
bool irMode(uint8_t value);
uint8_t romBank(uint8_t value);
uint8_t ramBank(uint8_t value);
uint8_t irRead(bool incomingLight);
}

#endif
