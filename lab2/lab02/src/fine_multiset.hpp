#pragma once

#include "set.hpp"
#include "std_set.hpp"

// ReSharper disable once CppUnusedIncludeDirective
#include <future>
#include <memory> // used by linux
#include <mutex>
#include <sstream>

/// The node used for the linked list implementation of a multiset in the
/// [`FineMultiset`] class. This struct is used for task 4.
struct FineMultisetNode {
    // A06: You can add or remove fields as needed.
    int value;
    int count;
    std::shared_ptr<FineMultisetNode> next;
    std::mutex lock;

    explicit FineMultisetNode(int value, int count)
        : value(value), count(count), next(nullptr), lock(std::mutex()) {
    }

    FineMultisetNode(int value,int count,  std::shared_ptr<FineMultisetNode> next)
        : value(value),
          next(std::move(next)), count(count), lock(std::mutex()) {
    }
};

/// A multiset implementation using a linked list with fine grained locking.
class FineMultiset : public Multiset {
private:
    // A06: You can add or remove fields as needed.
    std::shared_ptr<FineMultisetNode> head;
    EventMonitor<FineMultiset, StdMultiset, MultisetOperator> *monitor;

private:
    std::shared_ptr<FineMultisetNode> locate(int value) {
        // A02: Implement the `locate` function used for lazy synchronization.
        auto p = head;
        p->lock.lock();
        auto c = p->next;
        c->lock.lock();
        while (c->value < value) {
            p->lock.unlock();
            p = c;
            c = c->next;
            c->lock.lock();
        }
        return p;
    }

public:
    FineMultiset(EventMonitor<FineMultiset, StdMultiset, MultisetOperator> *monitor) : monitor(monitor) {
        head = std::make_shared<FineMultisetNode>(INT_MIN, 0,std::make_shared<FineMultisetNode>(INT_MAX, 0));
    }

    ~FineMultiset() override {
        // A06: Cleanup any memory that was allocated
    }

    int add(int elem) override {
        auto p = locate(elem);
        auto c = p->next;
        // this will release locks when function exits, adopt lock is because the mutex has been already locked
        std::lock_guard guardP(p->lock, std::adopt_lock);
        std::lock_guard guardC(c->lock, std::adopt_lock);
        if (c->value == elem) {
            c->count += 1;
            this->monitor->add(MultisetEvent(MultisetOperator::MSetAdd, elem, true));
            return true;
        }

        auto n = std::make_shared<FineMultisetNode>(elem, 1);
        n->next = c;
        p->next = n;
        this->monitor->add(MultisetEvent(MultisetOperator::MSetAdd, elem, true));
        return true;
    }

    int rmv(int elem) override {
        auto p = locate(elem);
        auto c = p->next;
        // this will release locks when function exits, adopt lock is because the mutex has been already locked
        std::lock_guard guardP(p->lock, std::adopt_lock);
        std::lock_guard guardC(c->lock, std::adopt_lock);
        if (c->value != elem) {
            this->monitor->add(MultisetEvent(MultisetOperator::MSetRemove, elem, false));
            return false;
        }
        if (c->count > 1) {
            c->count -= 1;
        }else {
            p->next = c->next;
        }
        this->monitor->add(MultisetEvent(MultisetOperator::MSetRemove, elem, true));
        return true;
    }

    int ctn(int elem) override {
        int result = 0;
        auto p = locate(elem);
        auto c = p->next;
        // this will release locks when function exits, adopt lock is because the mutex has been already locked
        std::lock_guard guardP(p->lock, std::adopt_lock);
        std::lock_guard guardC(c->lock, std::adopt_lock);
        if (c->value == elem) {
            this->monitor->add(MultisetEvent(MultisetOperator::MSetCount, elem, c->count));
            return c->count;
        } else {
            this->monitor->add(MultisetEvent(MultisetOperator::MSetCount, elem, 0));
            return false;
        }

        // A06: Add code to count how often elem is inside the set and update `result`.
        //      Also make sure, to insert the event inside the locked region of
        //      the linearization point.
        //
        //      There are different ways to implement a multiset ADT. The 
        //      skeleton code provides `monitor->add()`, `monitor->reserve()`,
        //      and `event->complete()` functions for this purpose. One can
        //      use only `monitor->add() or a combination of `monitor->reserve()`
        //      and `event->complete()` depending on their multiset 
        //      implementation. Go to `monitoring.hpp` and see the descriptions
        //      of these function.
        return result;
    }

    std::string print_state_str() {
        // A01: Optionally, add code to print the state. This is useful for debugging,
        // but not part of the assignment
        std::stringstream ss;
        ss << "FineMultiset { ";
        auto n = head;
        while (n->next != nullptr) {
            ss << "k:" << n->value << "+" << n->count<< ", ";
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
