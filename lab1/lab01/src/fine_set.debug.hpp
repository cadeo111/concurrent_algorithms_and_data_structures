#pragma once

#include "set.hpp"
#include "std_set.hpp"
#include <string>

#include <mutex>
#include <shared_mutex>
#include <utility>
#include <sstream>

#include "simple_set.hpp"


class FineSet;

class CustomMutex : public std::mutex {
    // std::atomic_bool locked = false;
    bool locked = false;

public:
    void lock() {
        mutex::lock();
        locked = true;
        // locked.store(true);
    }

    void unlock() {
        // locked.store(false);
        locked = false;
        mutex::unlock();
    }

    bool try_lock() {
        const auto result = mutex::try_lock();
        // locked.store(result);
        locked = result;
        return result;
    }

    bool is_locked() {
        return this->locked;
    }
};


/// The node used for the linked list implementation of a set in the [`FineSet`]
/// class. This struct is used for task 4.
struct FineSetNode {
    FineSetNode(int value, const std::shared_ptr<FineSetNode> &next)
        : value(value),
          next(next),
          mutex(CustomMutex()) {
    }

    // A04: You can add or remove fields as needed.
    int value;
    std::shared_ptr<FineSetNode> next;
    CustomMutex mutex;

    void lock() {
        this->mutex.lock();
    }

    void unlock() {
        this->mutex.unlock();
    }

    // std::lock_guard<std::mutex> guard() {
    //     lock();
    //     return adopt_guard();
    // }
    //
    // std::lock_guard<std::mutex> adopt_guard() {
    //     return {this->mutex, std::adopt_lock};
    // }

    bool is_locked() {
        return this->mutex.is_locked();
    }
};

inline std::mutex printMutex = std::mutex();

class BackupSet {
public:
    SimpleSetImpl s;
    std::mutex m;

public:
    explicit BackupSet(): s(SimpleSetImpl("BackupSet")), m(std::mutex()) {
    }

    bool add(int elem) {
        m.lock();
        auto r = s.add(elem);
        m.unlock();
        return r;
    }

    bool rmv(int elem) {
        m.lock();
        auto r = s.rmv(elem);
        m.unlock();
        return r;
    }

    bool ctn(const int elem) {
        m.lock();
        auto r = s.ctn(elem);
        m.unlock();
        return r;
    }

    std::shared_ptr<SimpleSetNode> head() {
        return this->s.get_head();
    }

    [[nodiscard]] std::string str_state_cmp() const {
        return s.state_str_cmp();
    }
};

bool compareFandB(FineSet f, BackupSet b);

/// A set implementation using a linked list with fine grained locking.
class FineSet : public Set {
private:
    // A04: You can add or remove fields as needed. Just having the `head`
    // pointer should be sufficient for task 4
public:
    std::shared_ptr<FineSetNode> head;
    // used to lock head b/c could be nullptr,
    CustomMutex pre_head_lock;
    std::shared_mutex action_lock;
    EventMonitor<FineSet, StdSet, SetOperator> *monitor;
    BackupSet bs;

public:
    FineSet(EventMonitor<FineSet, StdSet, SetOperator> *monitor) : monitor(monitor), bs(BackupSet()) {
        // A04: Initiate the internal state
    }

    ~FineSet() override {
        // A04: Cleanup any memory that was allocated
    }

    static void mark(const std::shared_ptr<FineSetNode> &node) {
    }

    bool add(int elem) override {
        std::shared_lock actionGaurd(action_lock);
        // If the list is empty add the value and return true
        pre_head_lock.lock();

        if (this->head == nullptr) {
            this->head = std::make_shared<FineSetNode>(elem, nullptr);
            verifyTrue(bs.add(elem));
            this->monitor->add(SetEvent(Add, elem, true));
            pre_head_lock.unlock();
            actionGaurd.unlock();
            verifyState(elem);
            return true;
        }
        // head is locked,

        auto head_ref = this->head;
        head_ref->lock();

        // one element ie [2]
        if (this->head->value == elem) {
            verifyFalse(bs.add(elem));
            this->monitor->add(SetEvent(Add, elem, false));
            head_ref->unlock();
            pre_head_lock.unlock();
            actionGaurd.unlock();
            verifyState(elem);
            return false;
        }
        // if the current head value is more than
        if (this->head->value > elem) {
            // std::unique_ptr<SimpleSetNode> el = ;
            this->head = std::make_shared<FineSetNode>(elem, this->head);
            verifyTrue(bs.add(elem));
            this->monitor->add(SetEvent(Add, elem, true));
            head_ref->unlock();
            pre_head_lock.unlock();
            actionGaurd.unlock();
            verifyState(elem);
            return true;
        }
        if (this->head->next == nullptr) {
            this->head->next = std::make_shared<FineSetNode>(elem, nullptr);
            verifyTrue(bs.add(elem));
            this->monitor->add(SetEvent(Add, elem, true));
            head_ref->unlock();
            pre_head_lock.unlock();
            actionGaurd.unlock();
            verifyState(elem);

            return true;
        }

        // 2+ elements
        auto prev = this->head; // locked above
        auto next = this->head->next;
        next->lock();

        pre_head_lock.unlock();

        // find the smallest value larger than elem, or the end of the list
        while (next != nullptr && elem > next->value) {
            const auto prevPrev = prev;
            prev = next; // this has been locked right after when next was assigned
            next = prev->next;
            if (next != nullptr) {
                next->lock();
            }
            prevPrev->unlock();
        }

        if (next == nullptr) {
            // there isn't any value larger than elem
            prev->next = std::make_shared<FineSetNode>(elem, nullptr);
            verifyTrue(bs.add(elem));
            this->monitor->add(SetEvent(Add, elem, true));
            prev->unlock();
            actionGaurd.unlock();
            verifyState(elem);

            return true;
        }

        if (next->value == elem) {
            verifyFalse(bs.add(elem));
            this->monitor->add(SetEvent(Add, elem, false));
            prev->unlock();
            next->unlock();
            actionGaurd.unlock();
            verifyState(elem);

            return false;
        }

        prev->next = std::make_shared<FineSetNode>(elem, next);
        verifyTrue(bs.add(elem));
        this->monitor->add(SetEvent(Add, elem, true));
        prev->unlock();
        next->unlock();
        actionGaurd.unlock();
        verifyState(elem);
        return true;
    }

    bool rmv(int elem) override {
        std::unique_lock actionGaurd(action_lock);
        // If the list is empty add the value and return true
        pre_head_lock.lock();
        if (this->head == nullptr) {
            verifyFalse(bs.rmv(elem));
            this->monitor->add(SetEvent(Remove, elem, false));
            pre_head_lock.unlock();
            actionGaurd.unlock();
            verifyState(elem);

            return false;
        }
        auto head_ref = this->head;
        head_ref->lock();
        // one element ie [2]
        if (this->head->value == elem) {
            if (this->head->next == nullptr) {
                this->head = nullptr;
                verifyTrue(bs.rmv(elem));
                this->monitor->add(SetEvent(Remove, elem, true));
                head_ref->unlock();
                pre_head_lock.unlock();
                actionGaurd.unlock();
                verifyState(elem);

                return true;
            }
            this->head = this->head->next;
            verifyTrue(bs.rmv(elem));
            this->monitor->add(SetEvent(Remove, elem, true));
            head_ref->unlock();
            pre_head_lock.unlock();
            actionGaurd.unlock();
            verifyState(elem);

            return true;
        }

        if (this->head->next == nullptr) {
            verifyFalse(bs.rmv(elem));
            this->monitor->add(SetEvent(Remove, elem, false));
            pre_head_lock.unlock();
            head_ref->unlock();
            actionGaurd.unlock();
            verifyState(elem);

            return false;
        }

        // 2+ elements
        auto prev = this->head; // locked above
        auto current = this->head->next;
        current->lock();
        pre_head_lock.unlock();

        // find the smallest value larger than elem, or the end of the list
        while (current != nullptr && elem != current->value) {
            const auto prevPrev = prev;
            prev = current;
            current = prev->next;
            if (current != nullptr) {
                current->lock();
            }
            prevPrev->unlock();
        }

        // made it to the end of the list without finding elem
        if (current == nullptr) {
            verifyFalse(bs.rmv(elem));
            this->monitor->add(SetEvent(Remove, elem, false));
            prev->unlock();
            actionGaurd.unlock();
            verifyState(elem);

            return false;
        }
        // found elem
        prev->next = current->next;
        verifyTrue(bs.rmv(elem));
        this->monitor->add(SetEvent(Remove, elem, true));
        prev->unlock();
        current->unlock();
        actionGaurd.unlock();
        verifyState(elem);

        return true;
    }

    bool ctn(int elem) override {
        std::shared_lock actionGaurd(action_lock);

        pre_head_lock.lock(); // lock the possibility of a nullptr
        auto node = this->head;
        if (node == nullptr) {
            this->monitor->add(SetEvent(Contains, elem, false));
            pre_head_lock.unlock();
            actionGaurd.unlock();
            return false;
        }
        node->lock();
        pre_head_lock.unlock();
        while (true) {
            // node should be locked
            if (node->value == elem) {
                this->monitor->add(SetEvent(Contains, elem, true));
                node->unlock();
                actionGaurd.unlock();
                return true;
            }
            if (node->next == nullptr || elem < node->next->value) {
                // sorted 0->inf
                this->monitor->add(SetEvent(Contains, elem, false));
                node->unlock();
                actionGaurd.unlock();
                return false;
            }
            auto prevNode = node;
            node = node->next;
            node->lock();
            prevNode->unlock();
        }
    }

    std::stringstream str_state() {
        std::stringstream ss;
        ss << "FineSet (H";
        if (this->pre_head_lock.is_locked()) {
            ss << "🔒";
        }
        ss << ") {";
        auto node = this->head;
        while (node != nullptr) {
            ss << node->value;
            if (node->is_locked()) {
                ss << "🔒";
            }
            if (node->next != nullptr) {
                ss << ", ";
            }
            node = node->next;
        }
        ss << "}";
        return ss;
    }

    std::string str_state_cmp() {
        std::stringstream ss;
        auto node = this->head;
        while (node != nullptr) {
            ss << node->value;
            if (node->next != nullptr) {
                ss << ",";
            }
            node = node->next;
        }
        return ss.str();
    }

    bool compareFandB(FineSet *f, BackupSet &b) {
        auto fstr = f->str_state_cmp();
        auto bstr = b.str_state_cmp();
        if (bstr.compare(fstr) != 0) {
            return false;
        }
        return true;
    }

    void verifyFalse(bool v) {
        if (v) {
            throw std::runtime_error("NOT FALSE");
        }
    }

    void verifyTrue(bool v) {
        if (!v) {
            throw std::runtime_error("NOT TRUE");
        }
    }

    void verifyState(int val) {
        bool valid = compareFandB(this, this->bs);
        if (!valid) {
            std::unique_lock actionSingleLock(action_lock);
            valid = compareFandB(this, this->bs);
            if (!valid) {
                this->print_state();
                this->bs.s.print_state();
                throw std::runtime_error("NOT VALID");
            }
            actionSingleLock.unlock();
        }
    }

    void print_state() override {
        // A02: Optionally, add code to print the state. This is useful for debugging,
        // but not part of the assignment.
        std::cout << str_state().str() << std::endl;
    }
};
