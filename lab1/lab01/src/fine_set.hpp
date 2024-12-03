#pragma once

#include "set.hpp"
#include "std_set.hpp"

#include <mutex>
#include <utility>
#include <sstream>


/// The node used for the linked list implementation of a set in the [`FineSet`]
/// class. This struct is used for task 4.
struct FineSetNode {
    FineSetNode(int value, const std::shared_ptr<FineSetNode> &next)
        : value(value),
          next(next),
          mutex(std::mutex()) {
    }

    // A04: You can add or remove fields as needed.
    int value;
    std::shared_ptr<FineSetNode> next;
    std::mutex mutex;
};

inline std::mutex printMutex = std::mutex();

/*
class MyUniqueLock {
public:
    std::string identifier;
    std::unique_lock<std::mutex> ul;

public:
    explicit MyUniqueLock(std::mutex &m, const std::string &identifier) {
        this->identifier = identifier;
        this->locking_pl();
        this->ul = std::unique_lock<std::mutex>(m);
        this->locked_pl();
    }


    explicit MyUniqueLock(const std::shared_ptr<FineSetNode> &node) {
        this->identifier = "val=" + std::to_string(node->value);
        locking_pl();
        this->ul = std::unique_lock<std::mutex>(node->mutex);
        locked_pl();
    }

    ~MyUniqueLock() {
        this->unlocking_pl();
    };

    MyUniqueLock(MyUniqueLock &&u) noexcept {
        // printMutex.lock();
        // std::cout << "tr:" << id() << " " << "Moving " << identifier << "(Locked:" << this->
        //         owns_lock() << ")" << std::endl;
        // printMutex.unlock();

        this->ul = std::unique_lock<std::mutex>(std::move(u.ul));
    }

    MyUniqueLock &operator=(MyUniqueLock &&u) noexcept {
        // printMutex.lock();
        // // std::cout << "tr:" << id() << " " << "= operator " << this->identifier << std::endl;
        // printMutex.unlock();
        unlocking_pl();
        this->identifier = u.identifier;
        locking_pl();
        this->ul = std::move(u.ul);
        locked_pl();
        return *this;
    }

    static std::string id() {
        std::stringstream ss;
        ss << std::this_thread::get_id();
        std::string st = ss.str();
        return st.substr(st.length() - 5, 2);
    }

    [[nodiscard]] bool owns_lock() const {
        return this->ul.owns_lock();
    }

    void locking_pl() {
        // printMutex.lock();
        // std::cout << "tr:" << id() << " " << "🐣Locking " << identifier << std::endl;
        // printMutex.unlock();
    }

    void unlocking_pl() {
        // printMutex.lock();
        // std::cout << "tr:" << id() << " " << "🐥Unlocked " << identifier << std::endl;
        // printMutex.unlock();
    }

    void locked_pl() {
        // printMutex.lock();
        // std::cout << "tr:" << id() << " " << "🥚Locked " << identifier << std::endl;
        // printMutex.unlock();
    }
};
*/




/// A set implementation using a linked list with fine grained locking.
class FineSet : public Set {
private:
    // A04: You can add or remove fields as needed. Just having the `head`
    // pointer should be sufficient for task 4
    std::shared_ptr<FineSetNode> head;
    // used to lock head b/c could be nullptr,
    std::mutex headLock;
    EventMonitor<FineSet, StdSet, SetOperator> *monitor;

public:
    FineSet(EventMonitor<FineSet, StdSet, SetOperator> *monitor) : monitor(monitor) {
        // A04: Initiate the internal state
    }

    ~FineSet() override {
        // A04: Cleanup any memory that was allocated
    }

    static void mark(const std::shared_ptr<FineSetNode> &node) {
    }

    bool add(int elem) override {
        // lock headlock, b/c we don't know if head is null
        auto headLock = std::unique_lock(this->headLock);
        // If the list is empty add the value and return true
        if (this->head == nullptr) {
            this->head = std::make_shared<FineSetNode>(elem, nullptr);
            // set point
            this->monitor->add(SetEvent(SetOperator::Add, elem, true));
            return true;
            // currentLock unlocks here b/c destructor
        }
        headLock.unlock();
        // since we know now the head is not null we can lock the lock owned by head, which automatically unlocks headLock
        // one element ie [2]
        auto prev = this->head;
        auto prevLock = std::unique_lock(this->head->mutex);
        if (this->head->value == elem) {
            this->monitor->add(SetEvent(SetOperator::Add, elem, false));
            return false;
            // currentLock unlocks here b/c destructor
        }
        // if the current head value is more than
        if (this->head->value > elem) {
            // std::unique_ptr<FineSetNode> el = ;
            this->head = std::make_shared<FineSetNode>(elem, this->head);
            this->monitor->add(SetEvent(SetOperator::Add, elem, true));
            return true;
            // currentLock unlocks here b/c destructor
        }
        if (this->head->next == nullptr) {
            this->head->next = std::make_shared<FineSetNode>(elem, nullptr);
            this->monitor->add(SetEvent(SetOperator::Add, elem, true));
            return true;
            // currentLock unlocks here b/c destructor
        }
        // 2+ elements

        auto next = this->head->next;
        auto nextLock = std::unique_lock(next->mutex);
        // std::lock(prevLock, nextLock);
        // find the smallest value larger than elem, or the end of the list
        while (next != nullptr && elem > next->value) {
            prev = next;
            prevLock.unlock();
            // this unlocks the lock for the old prev, and takes the existing lock for the next
            prevLock = std::unique_lock(std::move(nextLock));
            next = prev->next;
            if (next != nullptr) {
                // acquires/locks the "next" lock
                nextLock = std::unique_lock(next->mutex);
            }
        }
        if (next == nullptr) {
            // there isn't any value larger than elem
            prev->next = std::make_shared<FineSetNode>(elem, nullptr);
            this->monitor->add(SetEvent(SetOperator::Add, elem, true));
            return true;
            // prevLock unlocks here b/c destructor, nextLock shouldn't be locking anything
        }
        if (next->value == elem) {
            this->monitor->add(SetEvent(SetOperator::Add, elem, false));
            return false;
            // prevLock & nextLock unlocks here b/c destructor
        }
        prev->next = std::make_shared<FineSetNode>(elem, next);
        this->monitor->add(SetEvent(SetOperator::Add, elem, true));
        return true;
        // prevLock & nextLock unlocks here b/c destructor
    }

    bool rmv(int elem) override {
        // lock headlock, b/c we don't know if head is null
        std::unique_lock<std::mutex> currentLock = std::unique_lock(this->headLock);
        if (this->head == nullptr) {
            this->monitor->add(SetEvent(SetOperator::Remove, elem, false));
            return false;
        }
        // since we know now the head is not null we can lock the lock owned by head, which automatically unlocks headLock
        auto prev = this->head;
        auto prevLock = std::unique_lock(this->head->mutex);
        currentLock.unlock();

        // one element ie [2]
        if (this->head->value == elem) {
            this->head = this->head->next;
            this->monitor->add(SetEvent(SetOperator::Remove, elem, true));
            return true;
        }
        if (this->head->next == nullptr) {
            this->monitor->add(SetEvent(SetOperator::Remove, elem, false));
            return false;
        }
        // 2+ elements

        auto possible_value_to_remove = this->head->next;
        auto possibleLock = std::unique_lock(possible_value_to_remove->mutex);
        // find the smallest value larger than elem, or the end of the list
        while (possible_value_to_remove != nullptr && elem != possible_value_to_remove->value) {
            prev = possible_value_to_remove;
            // this unlocks the lock for the old prev, and takes the existing lock for possible as prev = possible
            prevLock.unlock();
            prevLock = std::unique_lock(std::move(possibleLock));
            possible_value_to_remove = prev->next;
            if (possible_value_to_remove != nullptr) {
                // acquires/locks the "possible_value_to_remove" lock
                possibleLock = std::unique_lock(possible_value_to_remove->mutex);
            }
        }
        // made it to the end of the list without finding elem
        if (possible_value_to_remove == nullptr) {
            this->monitor->add(SetEvent(SetOperator::Remove, elem, false));
            return false;
            // unlocks both possibleLock & prevLock b/c destructor
        }
        // found elem
        prev->next = possible_value_to_remove->next;
        this->monitor->add(SetEvent(SetOperator::Remove, elem, true));
        return true;
        // unlocks both possibleLock & prevLock b/c destructor
    }

    bool ctn(int elem) override {
        auto currentLock = std::unique_lock(this->headLock);
        if (this->head == nullptr) {
            this->monitor->add(SetEvent(SetOperator::Contains, elem, false));
            return false;
            // unlocks currentLock b/c destructor
        }

        auto node = this->head;
        while (true) {
            if (node->value == elem) {
                this->monitor->add(SetEvent(SetOperator::Contains, elem, true));
                return true;
                // unlocks currentLock b/c destructor
            }
            if (node->next == nullptr) {
                this->monitor->add(SetEvent(SetOperator::Contains, elem, false));
                return false;
                // unlocks currentLock b/c destructor
            }
            if (node->next->value > elem) {
                this->monitor->add(SetEvent(SetOperator::Contains, elem, false));
                return false;
                // unlocks currentLock b/c destructor
            }
            node = node->next;
            // unlock previous node and lock current node
            currentLock = std::unique_lock(node->mutex);
            // to stop unused variable warning
            (void) currentLock;
        }
    }

    std::stringstream str_state() {
        std::stringstream ss;
        ss << "FineSet" << " {";
        auto node = this->head;
        while (node != nullptr) {
            ss << node->value;
            if (node->next != nullptr) {
                ss << ", ";
            }
            node = node->next;
        }
        ss << "}";
        return ss;
    }

    void print_state() override {
        // A02: Optionally, add code to print the state. This is useful for debugging,
        // but not part of the assignment.
        std::cout << str_state().str() << std::endl;
    }
};
