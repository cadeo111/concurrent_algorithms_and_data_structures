#pragma once

#include "set.hpp"
#include "std_set.hpp"

/// The node used for the linked list implementation of a set in the [`SimpleSet`]
/// class. This struct is used for task 2
struct SimpleSetNode {
    // A02: You can add or remove fields as needed.
    int value;
    SimpleSetNode *next;
};

/// A simple set implementation using a linked list. This class shouldn't have
// any synchronization yet.
class SimpleSet : public Set {
private:
    // A02: You can add or remove fields as needed. Just having the `head`
    // pointer should be sufficient for task 2
    SimpleSetNode *head;
    EventMonitor<SimpleSet, StdSet, SetOperator> *monitor;

public:
    explicit SimpleSet(EventMonitor<SimpleSet, StdSet, SetOperator> *monitor) : monitor(monitor), head(nullptr) {
        // A02: Initiate the internal state
    }

    ~SimpleSet() override {
        // A02: Cleanup any memory that was allocated
        _del_next(head);
        head = nullptr;
    }

    static void _del_next(SimpleSetNode *node) {
        if (node->next != nullptr) {
            _del_next(node->next);
            delete node;
        }
    }

    bool _add(int elem) {
        // std::cout << "adding " << elem << "\n";
        // If the list is empty add the value and return true
        if (this->head == nullptr) {
            this->head = new SimpleSetNode{
                .value = elem,
                .next = nullptr
            };
            return true;
        }
        // one element ie [2]
        if (this->head->value == elem) {
            return false;
        }
        if (this->head->value > elem) {
            const auto el = new SimpleSetNode{
                .value = elem,
                .next = this->head
            };
            this->head = el;
            return true;
        }
        if (this->head->next == nullptr) {
            this->head->next = new SimpleSetNode{
                .value = elem,
                .next = this->head
            };
            return true;
        }
        // 2+ elements
        SimpleSetNode *prev = this->head;
        SimpleSetNode *next = this->head->next;
        // find the smallest value larger than elem, or the end of the list
        while (next != nullptr && elem > next->value) {
            prev = next;
            next = prev->next;
        }
        if (next == nullptr) {
            // there isn't any value larger than elem
            prev->next = new SimpleSetNode{
                .value = elem,
                .next = nullptr
            };
            return true;
        }
        if (next->value == elem) {
            return false;
        }

        prev->next = new SimpleSetNode{
            .value = elem,
            .next = next
        };
        return true;
    }


    bool add(int elem) override {
        // A02: Add code to insert the element into the set and update `result`.
        bool result = this->_add(elem);
        // validate_in_order();
        this->monitor->add(SetEvent(SetOperator::Add, elem, result));
        return result;
    }

    bool _rmv(int elem) {
        // std::cout << "removing " << elem << "\n";
        if (this->head == nullptr) {
            return false;
        }

        // one element ie [2]
        if (this->head->value == elem) {
            const SimpleSetNode *h = this->head;
            this->head = h->next;
            delete h;
            return true;
        }
        if (this->head->next == nullptr) {
            return false;
        }

        // 2+ elements
        SimpleSetNode *prev = this->head;
        SimpleSetNode *current = this->head->next;
        // find the smallest value larger than elem, or the end of the list
        while (current != nullptr && elem != current->value) {
            prev = current;
            current = prev->next;
        }
        // made it to the end of the list without finding elem
        if (current == nullptr) {
            return false;
        }
        // found elem
        const SimpleSetNode *toDelete = current;
        prev->next = current->next;
        delete toDelete;
        return true;
    }

    bool rmv(int elem) override {
        bool result = _rmv(elem);
        // validate_in_order();
        // A02: Add code to remove the element from the set and update `result`.
        this->monitor->add(SetEvent(SetOperator::Remove, elem, result));
        return result;
    }

    bool _ctn(int elem) {
        SimpleSetNode *node = this->head;
        while (true) {
            if (node->value == elem) return true;
            if (node->next == nullptr) return false;
            if (node->next->value > elem) return false;
            node = node->next;
        }
    }

    bool ctn(int elem) override {
        bool result = _ctn(elem);
        // A02: Add code to check if the element is inside the set and update `result`.
        this->monitor->add(SetEvent(SetOperator::Contains, elem, result));
        return result;
    }

    void validate_in_order() {
        if (this->head->next == nullptr) return;
        int last = this->head->value;
        SimpleSetNode *node = this->head->next;
        while (node != nullptr) {
            if (last >= node->value) {
                this->print_state();
                throw std::runtime_error("list out of order!");
            }
            last = node->value;
            node = node->next;
        }
    }

    void print_state() override {
        // A02: Optionally, add code to print the state. This is useful for debugging,
        // but not part of the assignment.
        std::cout << "SimpleSet {";
        SimpleSetNode *node = this->head;
        while (node != nullptr) {
            std::cout << node->value;
            if (node->next != nullptr) {
                std::cout << ", ";
            }
            node = node->next;
        }
        std::cout << "}";
    }
};
