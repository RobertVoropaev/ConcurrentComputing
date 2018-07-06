//
// Created by RobertVoropaev on 06.07.18.
//

#include <cstdlib>
#include <atomic>
#include <thread>

class TicketLock {
public:
    void Lock() {
        const size_t this_thread_ticket = next_free_ticket_.fetch_add(1);
        while (this_thread_ticket != owner_ticket_.load()) {
            std::this_thread::yield();
        }
    }

    bool TryLock() {
        size_t owner = owner_ticket_.load();
        return next_free_ticket_.compare_exchange_strong(owner, next_free_ticket_.load() + 1);
    }

    void Unlock() {
        owner_ticket_.store(owner_ticket_.load() + 1);
    }

private:
    std::atomic<size_t> next_free_ticket_{0};
    std::atomic<size_t> owner_ticket_{0};
};
