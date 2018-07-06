//
// Created by RobertVoropaev on 06.07.18.
//

#include <atomic>
#include <bits/move.h>

template <typename T>
class LockFreeStack {
private:
    struct Node {
        T item;
        Node *next;

        Node(T item) : item(std::move(item)), next(nullptr) {}
    };

    class LockFreeStackGarbage{
        struct GarbageNode{
            Node *item;
            GarbageNode *next;
            GarbageNode (Node* const &item) : item(item) {}
        };
    public:
        LockFreeStackGarbage() {}

        ~LockFreeStackGarbage() {
            GarbageNode *top = top_.load();
            while(top){
                GarbageNode *copy_top = top;
                top = top->next;
                delete copy_top->item;
                delete copy_top;
            }
        }

        void Push(Node *item){
            GarbageNode *new_node = new GarbageNode(item);
            new_node->next = top_.load();
            while(!top_.compare_exchange_strong(new_node->next, new_node)){}
        }

    private:
        std::atomic<GarbageNode*> top_{nullptr};
    };

public:
    LockFreeStack() {}

    ~LockFreeStack() {
        T item = T();
        while(Pop(item)){}
    }

    void Push(T item) {
        Node *new_node = new Node(item);
        new_node->next = top_;
        while(!top_.compare_exchange_strong(new_node->next, new_node)){}
    }

    bool Pop(T& item) {
        Node *curr_top = top_;
        while(true) {
            if (!curr_top) {
                return false;
            }
            if(top_.compare_exchange_strong(curr_top, curr_top->next)){
                item = curr_top->item;
                garbage_.Push(curr_top);
                return true;
            }
        }
    }

private:
    std::atomic<Node*> top_{nullptr};
    LockFreeStackGarbage garbage_;
};
