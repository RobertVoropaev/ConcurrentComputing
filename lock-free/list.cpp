//
// Created by RobertVoropaev on 06.07.18.
//

#include <cstdlib>
#include <limits>

//added tpcc library

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
class LockFreeLinkedSet {
private:
    struct Node {
        T key_;
        tpcc::lockfree::AtomicMarkedPointer<Node> next_;

        Node(const T& key, Node* next = nullptr) : key_(key), next_(next) {
        }

        Node* NextPointer() const {
            return next_.LoadPointer();
        }

        bool IsMarked() const {
            return next_.IsMarked();
        }
    };

    struct EdgeCandidate {
        Node* pred_;
        Node* curr_;

        EdgeCandidate(Node* pred, Node* curr) : pred_(pred), curr_(curr) {
        }
    };

public:
    explicit LockFreeLinkedSet(BumpPointerAllocator& allocator)
            : allocator_(allocator) {
        CreateEmptyList();
    }

    bool Insert(T key) {
        EdgeCandidate edge(nullptr, nullptr);
        Node* node_element = allocator_.New<Node>(key);
        while (true) {
            edge = Locate(key);
            node_element->next_.Store(edge.curr_);
            if (edge.curr_->key_ == key && !edge.curr_->IsMarked()) {
                return false;
            }
            if (edge.pred_->next_.CompareAndSet(node_element->next_.Load(), tpcc::lockfree::AtomicMarkedPointer<Node>(node_element).Load())) {
                return true;
            }
        }
    }

    bool Remove(const T& key) {
        EdgeCandidate edge(nullptr, nullptr);
        while (true) {
            edge = Locate(key);
            if (edge.curr_->key_ != key) {
                return false;
            } else {
                if (!edge.curr_->next_.TryMark(edge.curr_->NextPointer())) {
                    continue;
                }
                edge.pred_->next_.CompareAndSet(tpcc::lockfree::AtomicMarkedPointer<Node>(edge.curr_).Load(), tpcc::lockfree::AtomicMarkedPointer<Node>(edge.curr_->NextPointer()).Load());
                return true;
            }
        }
    }

    bool Contains(const T& key) const {
        Node* current_element = head_;
        while (current_element->key_ < key) {
            current_element = current_element->NextPointer();
        }
        return key == current_element->key_ &&
               !current_element->IsMarked();
    }

    size_t GetSize() const {
        size_t size{0};
        Node* current_element = head_;
        T max_element = KeyTraits<T>::UpperBound();
        while (current_element->key_ != max_element) {
            current_element = current_element->NextPointer();
            size++;
        }
        return size - 1;
    }

private:
    void CreateEmptyList() {
        // create sentinel nodes
        head_ = allocator_.New<Node>(TTraits::LowerBound());
        head_->next_ = allocator_.New<Node>(TTraits::UpperBound());
    }

    EdgeCandidate Locate(const T& key) const {
        Node* pred_element = head_;
        Node* current_element = head_;
        while (current_element->key_ < key) {
            pred_element = current_element;
            current_element = current_element->NextPointer();
            if (current_element->IsMarked() &&
                !pred_element->next_.CompareAndSet(tpcc::lockfree::AtomicMarkedPointer<Node>(current_element).Load(), tpcc::lockfree::AtomicMarkedPointer<Node>(current_element->NextPointer()).Load())) {
                current_element = head_;
            }
        }
        return EdgeCandidate{pred_element, current_element};
    }

private:
    BumpPointerAllocator& allocator_;
    Node* head_{nullptr};
};
