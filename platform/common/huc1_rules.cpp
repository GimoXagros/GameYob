#include "huc1_rules.h"

namespace huc1 {
bool irMode(uint8_t value) { return value == 0x0e; }
uint8_t romBank(uint8_t value) { return value & 0x3f; }
uint8_t ramBank(uint8_t value) { return value & 3; }
uint8_t irRead(bool incomingLight) { return incomingLight ? 0xc1 : 0xc0; }
}
