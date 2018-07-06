//
// Created by RobertVoropaev on 06.07.18.
//

#include <condition_variable>
#include <deque>

class QueueClosed : public std::runtime_error {
public:
    QueueClosed()
            : std::runtime_error("Queue closed for Puts") {}
};

template <typename T, class Container = std::deque<T>>
class BlockingQueue {
public:
    explicit BlockingQueue(const size_t capacity = 0):
            capacity_(capacity),
            closed_(false) {}

    void Put(T item) {
        std::unique_lock<std::mutex> lock(mutex_);

        while(IsFull() && !closed_) {
            can_add_.wait(lock);
        }

        if(closed_){
            throw QueueClosed();
        }


        items_.push_back(std::move(item));
        add_element_.notify_one();
    }

    bool Get(T& item) {
        std::unique_lock<std::mutex> lock(mutex_);

        while(IsEmpty() && !closed_) {
            add_element_.wait(lock);
        }

        if (IsEmpty() && closed_) {
            return false;
        }

        item = std::move(items_.front());
        items_.pop_front();
        can_add_.notify_one();
        return true;

    }

    void Close() {
        std::unique_lock<std::mutex> lock(mutex_);

        if (closed_)
            return;

        closed_ = true;
        can_add_.notify_all();
        add_element_.notify_all();
    }

private:

    bool IsFull() const {
        return capacity_ && (items_.size() == capacity_);
    }

    bool IsEmpty() const {
        return items_.empty();
    }

private:
    size_t capacity_;
    Container items_;
    bool closed_;

    std::mutex mutex_;
    std::condition_variable can_add_;
    std::condition_variable add_element_;
};
