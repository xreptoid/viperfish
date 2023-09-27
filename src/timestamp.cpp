#ifndef VIPERFISH_TIMESTAMP_CPP_
#define VIPERFISH_TIMESTAMP_CPP_

#include "timestamp.hpp"
#include <cmath>

namespace viperfish {

    void delay(double value) {
        unsigned micros = std::ceil(value * 1000 * 1000);
        unsigned seconds = micros / (1000 * 1000);
        micros = micros % (1000 * 1000);
        if (seconds) {
            sleep(seconds);
        }
        if (micros) {
            usleep(micros);
        }
    }
}

#endif
