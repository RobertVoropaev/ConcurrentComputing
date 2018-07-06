//
// Created by RobertVoropaev on 06.07.18.
//

#include <mutex>
#include <atomic>

//added bmp-alocator from tpcc library

class SpinLock {
public:
    void Lock() {
        do
            while(locked_.load());
        while (locked_.exchange(true));
    }

    void Unlock() {
        locked_.store(false);
    }

    // adapters for BasicLockable concept

    void lock() {
        Lock();
    }

    void unlock() {
        Unlock();
    }

private:
    std::atomic<bool> locked_{false};
};

////////////////////////////////////////////////////////////////////////////////

// don't touch this
template <typename T>
struct KeyTraits {
    static T LowerBound() {
        return std::numeric_limits<T>::min();
    }

    static T UpperBound() {
        return std::numeric_limits<T>::max();
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T, class TTraits = KeyTraits<T>>
class OptimisticLinkedSet {
private:
    struct Node {
        T key_;
        Node* next_;
        SpinLock spinlock_;
        bool marked_{false};

        Node(const T& key, Node* next = nullptr) :
                key_(key),
                next_(next) {}

        // use: auto node_lock = node->Lock();
        std::unique_lock<SpinLock> Lock() {
            return std::unique_lock<SpinLock>{spinlock_};
        }
    };

    struct EdgeCandidate {
        Node* pred_;
        Node* curr_;

        EdgeCandidate(Node* pred, Node* curr) :
                pred_(pred),
                curr_(curr) {}
    };

public:
    explicit OptimisticLinkedSet(BumpPointerAllocator& allocator) :
            allocator_(allocator) {
        CreateEmptyList();
    }

    bool Insert(T key) {
        EdgeCandidate edge(nullptr, nullptr);

        std::unique_lock<SpinLock> pred_lock;
        std::unique_lock<SpinLock> curr_lock;

        do{
            edge = Locate(key);

            pred_lock = edge.pred_->Lock();
            curr_lock = edge.curr_->Lock();

            if(!Validate(edge)){
                pred_lock.unlock();
                curr_lock.unlock();
            }
        } while (!Validate(edge));

        if(edge.curr_->key_ != key){
            Node *new_node = allocator_.New<Node>(key);

            new_node->next_ = edge.curr_;
            edge.pred_->next_ = new_node;

            size_++;
            return true;
        } else {
            return false;
        }
    }

    bool Remove(const T& key) {
        EdgeCandidate edge(nullptr, nullptr);

        std::unique_lock<SpinLock> pred_lock;
        std::unique_lock<SpinLock> curr_lock;

        do{
            edge = Locate(key);

            pred_lock = edge.pred_->Lock();
            curr_lock = edge.curr_->Lock();

            if(!Validate(edge)){
                pred_lock.unlock();
                curr_lock.unlock();
            }
        } while (!Validate(edge));

        if(edge.curr_->key_ == key){
            edge.pred_->next_ = edge.curr_->next_;
            edge.curr_->marked_ = true;

            size_--;
            return true;
        } else {
            return false;
        }
    }

    bool Contains(const T& key) const {
        EdgeCandidate edge = Locate(key);
        return edge.curr_->key_ == key &&
               !edge.curr_->marked_;
    }

    size_t GetSize() const {
        return size_;
    }

private:
    void CreateEmptyList() {
        // create sentinel nodes
        head_ = allocator_.New<Node>(TTraits::LowerBound());
        head_->next_ = allocator_.New<Node>(TTraits::UpperBound());
    }

    EdgeCandidate Locate(const T& key) const {
        Node *pred_ = head_;
        Node *curr_ = head_;
        while(curr_->key_ < key){
            pred_ = curr_;
            curr_ = curr_->next_;
        }
        return {pred_, curr_};
    }

    bool Validate(const EdgeCandidate& edge) const {
        return edge.pred_->next_ == edge.curr_ &&
               !edge.curr_->marked_ &&
               !edge.pred_->marked_;
    }

private:
    BumpPointerAllocator& allocator_;
    Node* head_{nullptr};
    size_t size_{0};
};
