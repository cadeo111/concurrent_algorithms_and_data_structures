#pragma once

#include "set.hpp"
#include "std_set.hpp"
// ReSharper disable once CppUnusedIncludeDirective
#include <memory> // used by linux
#include <mutex>
#include <sstream>

// A03: Copy your `FineSet` implementation from Lab 01 into this file and
// remove all references to the monitor. We want to benchmark the data
// structure and monitoring the performed operation would influence the
// results.

struct FineSetNode {
    std::mutex lock;
    int value;
    std::shared_ptr<FineSetNode> next;

    explicit FineSetNode(int value)
        : value(value), next(nullptr) {
    }

    explicit FineSetNode(int value, std::shared_ptr<FineSetNode> next)
        : value(value), next(std::move(next)) {
    }
};


class FineSet : public Set {
private:
    std::shared_ptr<FineSetNode> head;

    std::shared_ptr<FineSetNode> locate(int value) {
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
    FineSet() {
        head = std::make_shared<FineSetNode>(INT_MIN, std::make_shared<FineSetNode>(INT_MAX));
    };

    bool add(int elem) override {
        auto p = locate(elem);
        auto c = p->next;
        // this will release locks when function exits, adopt lock is because the mutex has been already locked
        std::lock_guard guardP(p->lock, std::adopt_lock);
        std::lock_guard guardC(c->lock, std::adopt_lock);
        if (c->value == elem) {
            return false;
        }
        auto n = std::make_shared<FineSetNode>(elem);
        n->next = c;
        p->next = n;
        return true;
    }

    bool rmv(int elem) override {
        auto p = locate(elem);
        auto c = p->next;
        // this will release locks when function exits, adopt lock is because the mutex has been already locked
        std::lock_guard guardP(p->lock, std::adopt_lock);
        std::lock_guard guardC(c->lock, std::adopt_lock);
        if (c->value != elem) {
            return false;
        }
        p->next = c->next;

        return true;
    }

    bool ctn(int elem) override {
        auto p = locate(elem);
        auto c = p->next;
        // this will release locks when function exits, adopt lock is because the mutex has been already locked
        std::lock_guard guardP(p->lock, std::adopt_lock);
        std::lock_guard guardC(c->lock, std::adopt_lock);
        if (c->value == elem) {
            return true;
        }
        return false;
    }


    std::string print_state_str() {
        // A01: Optionally, add code to print the state. This is useful for debugging,
        // but not part of the assignment
        std::stringstream ss;
        ss << "FineSet { ";
        auto n = head;
        while (n->next != nullptr) {
            ss << "k:" << n->value << ", ";
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
