#pragma once

#include <strstream>

#include "adt.hpp"

struct TreiberStackNode {
    // A01: You can add or remove fields as needed.
    int value;
    TreiberStackNode* next = nullptr;
};

class TreiberStack: public Stack {
private:
    // A04: You can add or remove fields as needed.
    std::atomic<TreiberStackNode*> top;
    std::atomic<int> count;

    EventMonitor<TreiberStack, StdStack, StackOperator>* monitor;
    /// This lock can be used around the CAS operation, to insert the
    /// operation into the monitor at the linearization point. This is
    /// just one way to do it, you can also try alternative options.
    std::mutex cas_lock;
public:
    TreiberStack(EventMonitor<TreiberStack, StdStack, StackOperator>* monitor)
        : monitor(monitor)
    {
        // A01: Initiate the internal state
    }

    ~TreiberStack() {
        // A01: Cleanup any memory that was allocated
    }

    int push(int value) override {
        int result = true;

        auto* new_node = new TreiberStackNode();
        new_node->value = value;


        while (true) {
            TreiberStackNode *t = top.load();
            new_node->next = t;
            cas_lock.lock();
            if (std::atomic_compare_exchange_strong(&top, &t, new_node)) {

                this->monitor->add(StackEvent(StackOperator::StackPush, value, result));
                this->count.fetch_add(1);
                cas_lock.unlock();
                break;
            }
            cas_lock.unlock();
        }

        // A01: Add code to insert the element at the top of the stack.
        //      Make sure, to insert the event at the linearization point.
        //      You can use the `cas_lock` to ensure that the event is
        //      inserted at the linearization point.

        return result;
    }

    int pop() override {
        int result = EMPTY_STACK_VALUE;
        while (true) {
            TreiberStackNode *t = top.load();
            cas_lock.lock();
            if (t == nullptr) {
                this->monitor->add(StackEvent(StackOperator::StackPop, NO_ARGUMENT_VALUE, result));
                cas_lock.unlock();
                break;
            }
            cas_lock.unlock();
            result = t->value;
            cas_lock.lock();
            if (std::atomic_compare_exchange_strong(&top, &t, t->next)) {
                this->monitor->add(StackEvent(StackOperator::StackPush,NO_ARGUMENT_VALUE , result));
                this->count.fetch_sub(1);
                cas_lock.unlock();
                break;
            }
            cas_lock.unlock();
        }

        // A01: Add code to pop the element at the top of the stack.
        //      Make sure, to insert the event at the linearization point.
        return result;
    }

    int size() override {
        cas_lock.lock();
        int result = count.load();
        this->monitor->add(StackEvent(StackOperator::StackSize, NO_ARGUMENT_VALUE, result));
        cas_lock.unlock();
        // A01: Add code to get the size of the stack.
        //      Make sure, to insert the event at the linearization point.
        return result;
    }

    char *print_str() {
        std::strstream out;
        out << "TreiberStack { ";
        auto n = this->top.load();
        while (n != nullptr) {
            out << "value: " << n->value << ", ";
            n = n->next;
        }
        out << " }";
        return out.str();
    }

    void print_state() override {
        std::cout << print_str();
    }
};