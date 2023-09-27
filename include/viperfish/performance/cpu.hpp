#ifndef INCLUDE_VIPERFISH_PERFORMANCE_CPU_HPP
#define INCLUDE_VIPERFISH_PERFORMANCE_CPU_HPP

#include <functional>
#include <thread>
#include <optional>
#include <chrono>
#include <iostream>
#include <unordered_set>

namespace viperfish::performance {

    class CpuGroup {
    public:

        CpuGroup(
            const std::unordered_set<int>& cpus
        );

        void attach(std::thread&) const;

    protected:
        std::unordered_set<int> cpus;

        #if __linux__
        cpu_set_t cpuset;
        #endif
    };

}

#endif