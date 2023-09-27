#include "utils/sleep.hpp"
#include <chrono>
#include <thread>

namespace viperfish::utils {

    void sleep_ms(int ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}
