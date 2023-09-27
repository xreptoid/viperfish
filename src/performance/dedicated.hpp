#ifndef INCLUDE_VIPERFISH_PERFORMANCE_DEDICATED_HPP
#define INCLUDE_VIPERFISH_PERFORMANCE_DEDICATED_HPP

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <chrono>
#include <iostream>

#include "./cpu.hpp"


namespace viperfish::performance {

    template<class TInput>
    class DedicatedThread {
    public:

        DedicatedThread(
            const std::function<void(TInput* in)>& execution_callback,
            CpuGroup* cpu_group = NULL
        )
                : execution_callback(execution_callback)
                , cpu_group(cpu_group)
        {
            thread = new std::thread([this]() mutable { thread_func(); });
            if (cpu_group) {
                cpu_group->attach(*thread);
            }
        }

        virtual ~DedicatedThread() {
            need_to_stop = true;
            wait();
            thread->join();
            delete thread;
        }

        bool request_execution(
            TInput* in,
            bool delete_in_after = false
        ) {
            if (has_request) {
                return false;
            }
            std::lock_guard execution_lock(execution_mutex);
            if (has_request) {
                return false;
            }
            this->in = in;
            this->delete_in_after = delete_in_after;
            has_request = true;
            {
                std::lock_guard lock(execution_cv_mutex);
            }
            execution_cv.notify_one();
            return true;
        }
        
        void request_execution_force(
            TInput* in,
            bool delete_in_after = false
        ) {
            while (true) {
                std::lock_guard execution_lock(execution_mutex);
                if (has_request) {
                    continue;
                }
                this->in = in;
                this->delete_in_after = delete_in_after;
                has_request = true;
                {
                    std::lock_guard lock(execution_cv_mutex);
                }
                execution_cv.notify_one();
                break;
            }
        }

        void wait() {
            bool was_request = false;
            {
                std::lock_guard execution_lock(execution_mutex);
                was_request = has_request;
            }
            if (was_request) {
                std::this_thread::sleep_for(std::chrono::microseconds(200));
                while (true) {
                    std::lock_guard execution_lock(execution_mutex);
                    if (!has_request) {
                        return;
                    }
                }
            }
        }

        bool is_free() {
            return !has_request;
        }

    private:
        std::function<void(TInput* in)> execution_callback;
        CpuGroup* cpu_group;

        std::thread* thread = NULL;
        std::mutex execution_mutex;
        volatile bool has_request = false;
        std::condition_variable execution_cv;
        std::mutex execution_cv_mutex;
        TInput* in = NULL;
        volatile bool delete_in_after = false;
        volatile bool need_to_stop = false;

        void thread_func() {
            while (!need_to_stop) {
                std::unique_lock cv_lock(execution_cv_mutex);
                execution_cv.wait(cv_lock, [] {return true; });
                cv_lock.unlock();
                {
                    std::lock_guard execution_lock(execution_mutex);
                    if (!has_request) {
                        continue;
                    }
                    execution_callback(in);
                    if (delete_in_after) {
                        delete_in_after = false;
                        delete in;
                    }
                    in = NULL;
                    has_request = false;
                }
            }
        }
    };
}

#endif 
