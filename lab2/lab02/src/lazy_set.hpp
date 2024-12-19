#pragma once

#include "set.hpp"
#include "std_set.hpp"

#include <mutex>
#include <atomic>

/// The node used for the linked list implementation of a set in the [`LazySet`]
/// class. This struct is used for task 3
struct LazySetNode {
    // A02: You can add or remove fields as needed.
    int value;

    /// The next pointer and the mark should actually be an std::atomic<OptimisticNextPointer*>.
    /// Using a normal pointer will probably work in most cases, but it is undefined behaviour to
    /// read and write to a non atomic data type simultaneously. In this lab it is okay to use a
    /// normal pointer but in future labs this will be a hard requirement.
    ///
    /// See this Stack Overflow thread for explonations of atomics in c++: <https://stackoverflow.com/questions/31978324/what-exactly-is-stdatomic>.
    /// See also the documentation for std::atomic: <https://en.cppreference.com/w/cpp/atomic/atomic>
    //std::atomic<bool> mark;
    //std::atomic<LazySetNode*> next;
    bool mark;
    LazySetNode* next;

    std::mutex lock;

    /// Default constructor which sets value and next to 0 and NULL respectively.
    LazySetNode() : value(0), mark(false), next(NULL) {}
};

/// A set implementation using a linked list with optimistic syncronization.
class LazySet: public Set {
private:
    // A02: You can add or remove fields as needed. Just having the `head`
    // pointer should be sufficient for this task
    LazySetNode* head;
public:
    LazySet()
    {
        // A02: Initiate the internal state
    }

    ~LazySet() override {
        // A02: Cleanup any memory that was allocated
        // This is optional for the lazy set since it might be tricky to implement and is out
        // of scope for this exercise, but remember to document this in your report.
    }

private:
    LazySetNode* locate(int value) {
        // A02: Implement the `locate` function used for lazy synchronization.
        return nullptr;
    }

public:
    bool add(int elem) override {
        bool result = false;
        // A02: Add code to insert the element into the set and update `result`.
        return result;
    }

    bool rmv(int elem) override {
        bool result = false;
        // A02: Add code to remove the element from the set and update `result`.
        return result;
    }

    bool ctn(int elem) override {
        bool result = false;
        // A02: Add code to check if the element is inside the set and update `result`.
        return result;
    }

    void print_state() override {
        // A02: Optionally, add code to print the state. This is useful for debugging,
        // but not part of the assignment
        std::cout << "LazySet {...}";
    }
};

