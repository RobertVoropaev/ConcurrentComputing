//
// Created by RobertVoropaev on 06.07.18.
//

#include <cstdlib>
#include <thread>
#include <atomic>
#include <cmath>
#include <vector>

class PetersonLock {
public:
    PetersonLock(){
        want[0].store(false);
        want[1].store(false);
        victim.store(0);
    }

    void Lock(size_t thread_index) {
        want[thread_index].store(true);
        victim.store(thread_index);
        while(want[1 - thread_index].load() && victim.load() == thread_index){
            std::this_thread::yield();
        }
    }

    void Unlock(size_t thread_index) {
        want[thread_index].store(false);
    }

private:
    std::array<std::atomic<bool>, 2> want;
    std::atomic<int> victim;
};

class TournamentTreeLock {
public:
    explicit TournamentTreeLock(size_t num_threads) {
        size_t size;
        if(num_threads > 1){
            size = size_t(pow(2, ceil(log2(num_threads))) - 1);
        } else {
            size = 1;
        }
        tree = std::vector<PetersonLock>(size);
    }

    void Lock(size_t thread_index) {
        size_t position = GetThreadLeaf(thread_index);
        while(position){
            tree[GetParent(position)].Lock(position % 2);
            position = GetParent(position);

        }
    }

    void Unlock(size_t thread_index) {
        std::vector<size_t> path;

        size_t position = GetThreadLeaf(thread_index);
        path.push_back(position);
        while(position){
            path.push_back(GetParent(position));
            position = GetParent(position);
        }

        for(size_t i = path.size() - 1; i >= 1; i--){
            tree[path[i]].Unlock(path[i - 1] % 2);
        }
    }

private:
    size_t GetParent(size_t node_index) const {
        return (node_index - 1) / 2;
    }

    size_t GetLeftChild(size_t node_index) const {
        return (node_index * 2) + 1;
    }

    size_t GetRightChild(size_t node_index) const {
        return (node_index * 2) + 2;
    }

    size_t GetThreadLeaf(size_t thread_index) const {
        return tree.size() + thread_index;
    }

private:
    std::vector<PetersonLock> tree;
};
