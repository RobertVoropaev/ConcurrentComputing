//
// Created by RobertVoropaev on 06.07.18.
//

#include <mutex>
#include <condition_variable>

class CyclicBarrier {
public:
    explicit CyclicBarrier(const size_t num_threads)
            : num_threads_(num_threads) {
        current_threads_ = 0;
        is_filling_ = true;
    }

    void PassThrough() {
        std::unique_lock<std::mutex> lock(mutex_);
        while(!is_filling_){
            cv_empty_.wait(lock);
        }
        current_threads_++;
        if(current_threads_ != num_threads_){
            while(is_filling_){
                cv_full_.wait(lock);
            }
        } else {
            is_filling_ = false;
            cv_full_.notify_all();
        }
        current_threads_--;
        if(current_threads_ == 0){
            is_filling_ = true;
            cv_empty_.notify_all();
        }
    }

private:
    size_t num_threads_;
    size_t current_threads_;
    bool is_filling_;
    std::condition_variable cv_empty_;
    std::condition_variable cv_full_;
    std::mutex mutex_;
};
