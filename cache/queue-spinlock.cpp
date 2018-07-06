//
// Created by RobertVoropaev on 06.07.18.
//

#include <atomic>
#include <thread>

class QueueSpinLock {
public:
    class LockGuard {
    public:
        explicit LockGuard(QueueSpinLock& spinlock) : spinlock_(spinlock) {
            AcquireLock();
        }

        ~LockGuard() {
            ReleaseLock();
        }

    private:
        void AcquireLock() {
            LockGuard* prev_tail = spinlock_.wait_queue_tail_.exchange(this);
            if(prev_tail != nullptr) {
                prev_tail->next_.store(this);
                while(!is_owner_.load()){
                    std::this_thread::yield();
                }
            }
        }

        void ReleaseLock() {
            LockGuard* guard = this;
            if(!spinlock_.wait_queue_tail_.compare_exchange_strong(guard, nullptr)){
                while(next_.load() == nullptr){
                    std::this_thread::yield();
                }
                next_.load()->is_owner_.store(true);
            }
        }

    private:
        QueueSpinLock& spinlock_;

        std::atomic<bool> is_owner_{false};
        std::atomic<LockGuard*> next_{nullptr};
    };

private:
    // tail of intrusive list of LockGuards
    std::atomic<LockGuard*> wait_queue_tail_{nullptr};
};
