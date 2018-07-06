//
// Created by RobertVoropaev on 06.07.18.
//

#include <condition_variable>

class Semaphore {
public:
    explicit Semaphore(const size_t count = 0)
            : count_(count) {}

    void Acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        while(count_ == 0){
            has_tokens_.wait(lock);
        }
        count_--;
    }

    void Release() {
        std::lock_guard<std::mutex> lock(mutex_);
        count_++;
        has_tokens_.notify_one();
    }

private:
    size_t count_;
    std::mutex mutex_;
    std::condition_variable has_tokens_;
};