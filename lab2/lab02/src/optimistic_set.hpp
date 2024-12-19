#pragma once

#include "set.hpp"
#include "std_set.hpp"

#include <mutex>
#include <atomic>
#include <utility>

/// The node used for the linked list implementation of a set in the [`OptimisticSet`]
/// class. This struct is used for task 3
struct OptimisticSetNode {
    // A01: You can add or remove fields as needed.
    int value;

    /// The next pointer should actually be an std::atomic<OptimisticNextPointer*>.  Using a normal
    /// pointer will probably work in most cases, but it is undefined behaviour to read and write to
    /// a non atomic data type simultaneously. In this lab it is okay to use a normal pointer but in
    /// future labs this will be a hard requirement.
    ///
    /// See this Stack Overflow thread for explonations of atomics in c++: <https://stackoverflow.com/questions/31978324/what-exactly-is-stdatomic>.
    /// See also the documentation for std::atomic: <https://en.cppreference.com/w/cpp/atomic/atomic>
    std::shared_ptr<OptimisticSetNode> next;

    std::mutex m;

    void lock() {
        this->m.lock();
    }

    void unlock() {
        this->m.unlock();
    }


    /// Default constructor which sets value and next to 0 and NULL respectively.
    OptimisticSetNode() : value(0), next(nullptr) {
    }

    explicit OptimisticSetNode(const int value) : value(value), next(nullptr) {
    }

    OptimisticSetNode(const int value, std::shared_ptr<OptimisticSetNode> next) : value(value), next(std::move(next)) {
    }
};

/// A set implementation using a linked list with optimistic syncronization.
class OptimisticSet : public Set {
private:
    // A01: You can add or remove fields as needed. Just having the `head`
    // pointer should be sufficient for this task
    std::shared_ptr<OptimisticSetNode> head;

public:
    OptimisticSet() {
        // add dummy node at the head that is lowes possible value and end with highest possible value
        head = std::make_shared<OptimisticSetNode>(INT_MIN, std::make_shared<OptimisticSetNode>(INT_MAX));

        // A01: Initiate the internal state
    }

    ~OptimisticSet() override {
        // A01: Cleanup any memory that was allocated
        // This is optional for the optimistic set since it might be tricky to implement and is out
        // of scope for the exercise, but remember to document this in your report.
    }

private:
    bool validate(const std::shared_ptr<OptimisticSetNode> &p, const std::shared_ptr<OptimisticSetNode> &c) {
        auto n = head;
        while (n->value < p->value) {
            n = n->next;
        }

        if (n->value == p->value) {
            if ( p->next->value == c->value) {
                return true;
            }
        }
        // A01: Implement the `validate` function used during
        // optimistic synchronization.
        return false;
    }

public:
    bool add(int elem) override {
        while (true) {
            auto p = head;
            auto c = p->next;
            while (c->value <= elem) {
                p = c;
                c = c->next;
            }

            std::scoped_lock lock(p->m, c->m);
            if (validate(p, c)) {
                if (c->value == elem) {
                    return false;
                }
                const auto n = std::make_shared<OptimisticSetNode>(elem);
                n->next = c;
                p->next = n;
                return true;
            }
            // p->unlock();
            // c->unlock();
        }
    }

    bool rmv(int elem) override {
        while (true) {
            auto p = head;
            auto c = p->next;
            while (c->value < elem) {
                p = c;
                c = c->next;
            }
            std::scoped_lock lock(p->m, c->m); // this should unlock automatically when it goes out of scope
            if (validate(p, c)) {
                if (c->value == elem) {
                    p->next = c->next;
                    return true;
                }
                return false;
            }
        }
    }

    bool ctn(int elem) override {
        while (true) {
            auto p = head;
            auto c = p->next;
            while (c->value < elem) {
                p = c;
                c = c->next;
            }
            std::scoped_lock lock(p->m, c->m); // this should unlock automatically when it goes out of scope
            if (validate(p, c)) {
                if (c->value == elem) {
                    return true;
                }
                return false;
            }
        }
    }

    void print_state() override {
        // A01: Optionally, add code to print the state. This is useful for debugging,
        // but not part of the assignment
        std::cout << "OptimisticSet {...}";
    }
};
