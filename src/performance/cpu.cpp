#include "performance/cpu.hpp"

#include <sched.h>


namespace viperfish::performance {

    CpuGroup::CpuGroup(
            const std::unordered_set<int>& cpus
    )
            : cpus(cpus)
    {
        #if __linux__
        CPU_ZERO(&cpuset);
        for (const int i_cpu: cpus) {
            CPU_SET(i_cpu, &cpuset);
        }
        #endif
    }

    void CpuGroup::attach(std::thread& t) const {
        #if __linux__
        int rc = pthread_setaffinity_np(
            t.native_handle(),
            sizeof(cpu_set_t),
            &cpuset
        );
        if (rc != 0) {
            std::cout << "Error calling pthread_setaffinity_np: " << rc << std::endl;
        }
        #endif
    }
}