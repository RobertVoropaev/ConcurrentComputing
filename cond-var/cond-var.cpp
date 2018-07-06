//
// Created by RobertVoropaev on 06.07.18.
//

#include <mutex>
#include <atomic>

//added futex from tpcc library

class ConditionVariable {
public:
    ConditionVariable()
            : futex_(signal_count_) {}

    template <class Mutex>
    void Wait(Mutex& mutex) {
        mutex.unlock();
        thread_count_.fetch_add(1);
        futex_.Wait(0);
        mutex.lock();
    }

    void NotifyOne() {
        std::lock_guard<std::mutex> lock(mutex_);
        signal_count_.fetch_add(1);
        thread_count_.fetch_sub(1);
        futex_.WakeOne();
    }

    void NotifyAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        signal_count_.fetch_add(thread_count_.load());
        thread_count_.store(0);
        futex_.WakeAll();
    }

private:
    std::atomic<uint32_t> signal_count_{0};
    std::atomic<uint32_t> thread_count_{0};
    std::mutex mutex_;
    tpcc::Futex futex_;
};
