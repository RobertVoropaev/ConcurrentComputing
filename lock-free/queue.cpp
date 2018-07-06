//
// Created by RobertVoropaev on 06.07.18.
//

#include <cstdlib>
#include <atomic>
#include <bits/move.h>

template <typename T>
class LockFreeQueue {
    struct Node {
        T item_;
        std::atomic<Node*> next_{nullptr};

        Node() {}

        explicit Node(T item, Node* next = nullptr)
                : item_(std::move(item)),
                  next_(next) {}
    };

public:
    LockFreeQueue() {
        Node* dummy = new Node{};
        head_ = dummy;
        tail_ = dummy;
        garbage_head_.store(dummy);
    }

    ~LockFreeQueue() {
        Node* curr_tail = tail_.load();
        while(garbage_head_.load() != curr_tail){
            Node* del_node = garbage_head_.load();
            garbage_head_.store(garbage_head_.load()->next_);
            delete del_node;
        }
        delete garbage_head_.load();
    }

    void Enqueue(T item) {
        counter_.fetch_add(1);

        Node *new_tail = new Node(item);
        Node *curr_tail;


        while(true){
            curr_tail = tail_.load();
            if(!curr_tail->next_.load()){
                Node *ptr = nullptr;
                if(curr_tail->next_.compare_exchange_strong(ptr, new_tail)){
                    break;
                }
            } else {
                tail_.compare_exchange_strong(curr_tail, curr_tail->next_.load());
            }
        }
        tail_.compare_exchange_strong(curr_tail, new_tail);
        counter_.fetch_sub(1);
    }

    bool Dequeue(T& item) {
        counter_.fetch_add(1);

        Node *curr_tail = tail_;
        Node *curr_head = head_;

        if (counter_.load() == 1) {
            while (garbage_head_.load() != curr_head) {
                Node* del_node = garbage_head_.load();
                garbage_head_.store(garbage_head_.load()->next_.load());
                delete del_node;
            }
        }

        while(true){
            if(curr_head == curr_tail){
                if(!curr_tail->next_.load()){
                    counter_.fetch_sub(1);
                    return false;
                } else {
                    tail_.compare_exchange_strong(curr_head, curr_head->next_.load());
                }
            } else {
                if (head_.compare_exchange_strong(curr_head, curr_head->next_.load())){
                    item = curr_head->next_.load()->item_;
                    counter_.fetch_sub(1);
                    return true;
                }
            }
        }
    }

private:
    std::atomic<Node*> head_{nullptr};
    std::atomic<Node*> tail_{nullptr};

    std::atomic<Node*> garbage_head_{nullptr};
    std::atomic<size_t> counter_{0};
};
