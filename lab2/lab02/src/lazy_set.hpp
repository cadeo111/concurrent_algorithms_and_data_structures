#pragma once

#include "set.hpp"
#include "std_set.hpp"
#include <sstream>

// ReSharper disable once CppUnusedIncludeDirective
#include <memory> // used on linux
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
    std::atomic<bool> mark;
    // std::atomic<LazySetNode*> next;
    std::shared_ptr<LazySetNode> next;

    std::mutex lock;

    /// Default constructor which sets value and next to 0 and NULL respectively.
    LazySetNode() : value(0), mark(false), next(nullptr) {
    }

    explicit LazySetNode(int value) : value(value), mark(false), next(nullptr) {
    }

    LazySetNode(int value, std::shared_ptr<LazySetNode> next) : value(value), mark(false), next(std::move(next)) {
    }
};

/// A set implementation using a linked list with optimistic syncronization.
class LazySet : public Set {
private:
    // A02: You can add or remove fields as needed. Just having the `head`
    // pointer should be sufficient for this task
    std::shared_ptr<LazySetNode> head;

public:
    LazySet() {
        // add start and end for easier logic
        head = std::make_shared<LazySetNode>(INT_MIN, std::make_shared<LazySetNode>(INT_MAX));
        // A02: Initiate the internal state
    }

    ~LazySet() override {
        // A02: Cleanup any memory that was allocated
        // This is optional for the lazy set since it might be tricky to implement and is out
        // of scope for this exercise, but remember to document this in your report.
    }

private:
    std::shared_ptr<LazySetNode> locate(int value) {
        // A02: Implement the `locate` function used for lazy synchronization.
        while (true) {
            auto p = head;
            auto c = p->next;
            while (c->value < value) {
                p = c;
                c = c->next;
            }
            p->lock.lock();
            c->lock.lock();
            if (!p->mark && !c->mark && p->next->value == c->value) {
                return p;
            }
            p->lock.unlock();
            c->lock.unlock();
        }
    }

public:
    bool add(int elem) override {
        // both p and c are locked after locate
        auto p = locate(elem);
        auto c = p->next;
        // this will release locks when function exits, adopt lock is because the mutex has been already locked
        std::lock_guard guardP(p->lock, std::adopt_lock);
        std::lock_guard guardC(c->lock, std::adopt_lock);
        if (c->value != elem) {
            const auto n = std::make_shared<LazySetNode>(elem);
            n->next = c;
            p->next = n;
            return true;
        }
        return false;
    }

    bool rmv(int elem) override {
        auto p = locate(elem);
        auto c = p->next;
        // this will release locks when function exits, adopt lock is because the mutex has been already locked
        std::lock_guard guardP(p->lock, std::adopt_lock);
        std::lock_guard guardC(c->lock, std::adopt_lock);
        if (c->value == elem) {
            c->mark = true;
            const auto n = c->next;
            p->next = n;
            return true;
        }
        return false;
    }

    bool ctn(int elem) override {
        auto c = head;
        while (c->value < elem) {
            c = c->next;
        }
        bool b = c->mark.load();
        if (!b && c->value == elem) {
            return true;
        }
        return false;
    }
    std::string print_state_str() {
        // A01: Optionally, add code to print the state. This is useful for debugging,
        // but not part of the assignment
        std::stringstream ss;
        ss << "LazySet { ";
        auto n = head;
        while (n->next != nullptr) {
            ss << "value: " << n->value << ", ";
            n = n->next;
        }
        ss << " }";
        return ss.str();
    }

    void print_state() override {
        // A01: Optionally, add code to print the state. This is useful for debugging,
        // but not part of the assignment
        std::cout << print_state_str();
    }
};
