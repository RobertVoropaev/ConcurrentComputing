cmake_minimum_required(VERSION 3.9)
project(ConcurrentComputing)

set(CMAKE_CXX_STANDARD 14)

add_executable(ConcurrentComputing mutex/ticket-lock.cpp mutex/tournament-tree.cpp cond-var/byclic-varrier.cpp cond-var/reader-writer-lock.cpp cond-var/semaphore.cpp cond-var/blocking-queue.cpp cond-var/cond-var.cpp fine-grained/hash-table.cpp fine-grained/optimistic-list.cpp cache/queue-spinlock.cpp "lock-free/stack.cpp" lock-free/queue.cpp lock-free/list.cpp)