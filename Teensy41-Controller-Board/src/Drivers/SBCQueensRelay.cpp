#include "Drivers/SBCQueensRelay.h"

#include <arduino_freertos.h>

namespace SBCQueens {

void relay_init(Relay& mod) {

    pinMode(mod.PIN, arduino::OUTPUT);
    digitalWrite(mod.PIN, mod.Value > 0);

}

void relay_update(Relay& mod) {
    digitalWrite(mod.PIN, mod.Value > 0);
}

} // namespace SBCQueens
