//
// Created by RobertVoropaev on 06.07.18.
//

#include <mutex>
#include <atomic>
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


class ReaderWriterLock {
public:

    ReaderWriterLock() :
            reader_counter_(0),
            semaphore_(1) {}

    // reader section / shared ownership

    void ReaderLock() {
        std::lock_guard<std::mutex> lock(worked_mutex);
        if(reader_counter_.fetch_add(1) == 0){
            semaphore_.Acquire();
        }
    }

    void ReaderUnlock() {
        std::lock_guard<std::mutex> lock(worked_mutex);
        if(reader_counter_.fetch_sub(1) == 1){
            semaphore_.Release();
        }
    }

    // writer section / exclusive ownership

    void WriterLock() {
        semaphore_.Acquire();
    }

    void WriterUnlock() {
        semaphore_.Release();
    }

private:
    std::atomic<size_t> reader_counter_;
    std::mutex worked_mutex;
    Semaphore semaphore_;
};
