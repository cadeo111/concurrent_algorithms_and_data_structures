#pragma once

#include "set.hpp"

#include <utility>
#include "std_set.hpp"

/// The node used for the linked list implementation of a set in the [`SimpleSet`]
/// class. This struct is used for task 2
struct SimpleSetNode {
    // A02: You can add or remove fields as needed.
    int value;
    std::shared_ptr<SimpleSetNode> next;
};

class SimpleSetImpl {
private:
    std::shared_ptr<SimpleSetNode> head;
    std::string label;

public:
    explicit SimpleSetImpl(std::string label)
        : label(std::move(label)) {
    }

    ~SimpleSetImpl() = default;

    bool add(int elem) {
        // std::cout << "adding " << elem << "\n";
        // If the list is empty add the value and return true
        if (this->head == nullptr) {
            this->head = std::make_shared<SimpleSetNode>(SimpleSetNode{
                .value = elem,
                .next = nullptr
            });
            return true;
        }
        // one element ie [2]
        if (this->head->value == elem) {
            return false;
        }
        // if the current head value is more than
        if (this->head->value > elem) {
            // std::unique_ptr<SimpleSetNode> el = ;
            this->head = std::make_shared<SimpleSetNode>(SimpleSetNode{
                .value = elem,
                .next = std::move(this->head)
            });
            return true;
        }
        if (this->head->next == nullptr) {
            this->head->next = std::make_shared<SimpleSetNode>(SimpleSetNode{
                .value = elem,
                .next = nullptr
            });
            return true;
        }
        // 2+ elements
        auto prev = this->head;
        auto next = this->head->next;
        // find the smallest value larger than elem, or the end of the list
        while (next != nullptr && elem > next->value) {
            prev = next;
            next = prev->next;
        }
        if (next == nullptr) {
            // there isn't any value larger than elem
            prev->next = std::make_shared<SimpleSetNode>(SimpleSetNode{
                .value = elem,
                .next = nullptr
            });
            return true;
        }
        if (next->value == elem) {
            return false;
        }

        prev->next = std::make_shared<SimpleSetNode>(SimpleSetNode{
            .value = elem,
            .next = next
        });
        return true;
    }

    bool rmv(int elem) {
        // std::cout << "removing " << elem << "\n";
        if (this->head == nullptr) {
            return false;
        }

        // one element ie [2]
        if (this->head->value == elem) {
            this->head = this->head->next;
            return true;
        }
        if (this->head->next == nullptr) {
            return false;
        }

        // 2+ elements
        auto prev = this->head;
        auto current = this->head->next;
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
        prev->next = current->next;
        return true;
    }

    bool ctn(const int elem) {
        auto node = this->head;
        while (true) {
            if (node->value == elem) return true;
            if (node->next == nullptr) return false;
            if (node->next->value > elem) return false;
            node = node->next;
        }
    }

    void validate_in_order() {
        if (this->head->next == nullptr) return;
        int last = this->head->value;
        auto node = this->head->next;
        while (node != nullptr) {
            if (last >= node->value) {
                this->print_state();
                throw std::runtime_error("list out of order!");
            }
            last = node->value;
            node = node->next;
        }
    }

    void print_state() const {
        // A02: Optionally, add code to print the state. This is useful for debugging,
        // but not part of the assignment.
        std::cout << label << " {";
        auto node = this->head;
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
/// A simple set implementation using a linked list. This class shouldn't have
// any synchronization yet.
class SimpleSet : public Set {
private:
    // A02: You can add or remove fields as needed. Just having the `head`
    // pointer should be sufficient for task 2
    SimpleSetImpl set;
    EventMonitor<SimpleSet, StdSet, SetOperator> *monitor;

public:
    explicit SimpleSet(EventMonitor<SimpleSet, StdSet, SetOperator> *monitor) : set(SimpleSetImpl("SimpleSet")), monitor(monitor) {
        // A02: Initiate the internal state
    }

    ~SimpleSet() override = default;

    bool add(int elem) override {
        // A02: Add code to insert the element into the set and update `result`.
        bool result = set.add(elem);
        // validate_in_order();
        this->monitor->add(SetEvent(SetOperator::Add, elem, result));
        return result;
    }


    bool rmv(const int elem) override {
        bool result = set.rmv(elem);
        // validate_in_order();
        // A02: Add code to remove the element from the set and update `result`.
        this->monitor->add(SetEvent(SetOperator::Remove, elem, result));
        return result;
    }


    bool ctn(int elem) override {
        bool result = set.ctn(elem);
        // A02: Add code to check if the element is inside the set and update `result`.
        this->monitor->add(SetEvent(SetOperator::Contains, elem, result));
        return result;
    }

    void print_state() override {
        // A02: Optionally, add code to print the state. This is useful for debugging,
        // but not part of the assignment.
        this->set.print_state();
    }
};
