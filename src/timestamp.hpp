#ifndef VIPERFISH_TIMESTAMP_HPP
#define VIPERFISH_TIMESTAMP_HPP

#include <cstdint>
#include <unistd.h>
#include <cmath>
#include <chrono>

namespace viperfish {

    inline
    std::uint64_t get_current_ts()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
    }

    inline
    std::uint64_t get_current_ts_micro()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }

    void delay(double value);
}

#endif 
