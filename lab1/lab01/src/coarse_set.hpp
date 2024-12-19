#pragma once

#include "set.hpp"
#include "std_set.hpp"

#include <mutex>

#include "simple_set.hpp"

// /// The node used for the linked list implementation of a set in the [`CoarseSet`]
// /// class. This struct is used for task 3
// struct CoarseSetNode {
//     // A03: You can add or remove fields as needed.
//     int value;
//     std::unique_ptr<CoarseSetNode> next;
// };





/// A set implementation using a linked list with coarse grained locking.
class CoarseSet : public Set {
private:
    // A03: You can add or remove fields as needed. Just having the `head`
    // pointer and the `lock` should be sufficient for task 3
    SimpleSetImpl unsafe_set;
    std::mutex lock;
    EventMonitor<CoarseSet, StdSet, SetOperator> *monitor;

public:
    CoarseSet(EventMonitor<CoarseSet, StdSet, SetOperator> *monitor) : unsafe_set(SimpleSetImpl("CoarseSet")),
                                                                       monitor(monitor) {
        // A03: Initiate the internal state
    }

    ~CoarseSet() override {
        // A03: Cleanup any memory that was allocated
    }


    bool add(int elem) override {
        lock.lock();
        bool result = unsafe_set.add(elem);
        // A03: Add code to insert the element into the set and update `result`.
        //      Also make sure, to insert the event inside the locked region of
        //      the linearization point.
        this->monitor->add(SetEvent(SetOperator::Add, elem, result));
        lock.unlock();
        return result;
    }

    bool rmv(int elem) override {
        lock.lock();
        bool result = unsafe_set.rmv(elem);
        // A03: Add code to remove the element from the set and update `result`.
        //      Also make sure, to insert the event inside the locked region of
        //      the linearization point.
        this->monitor->add(SetEvent(SetOperator::Remove, elem, result));
        lock.unlock();
        return result;
    }

    bool ctn(int elem) override {
        lock.lock();
        bool result = unsafe_set.ctn(elem);
        // A03: Add code to check if the element is inside the set and update `result`.
        //      Also make sure, to insert the event inside the locked region of
        //      the linearization point.
        this->monitor->add(SetEvent(SetOperator::Contains, elem, result));
        lock.unlock();
        return result;
    }

    void print_state() override {
        // A03: Optionally, add code to print the state. This is useful for debugging,
        // but not part of the assignment
        unsafe_set.print_state();
    }
};
