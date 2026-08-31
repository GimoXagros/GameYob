#include "huc1_rules.h"

#include <assert.h>

int main() {
    assert(huc1::irMode(0x0e));
    assert(!huc1::irMode(0x0a));
    assert(!huc1::irMode(0x00));
    assert(huc1::romBank(0xff) == 0x3f);
    assert(huc1::romBank(0x40) == 0);
    assert(huc1::ramBank(0xff) == 3);
    assert(huc1::irRead(false) == 0xc0);
    assert(huc1::irRead(true) == 0xc1);
    return 0;
}
