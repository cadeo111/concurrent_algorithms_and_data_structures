#pragma once

#include "monitoring.hpp"
#include "adt.hpp"

const uintptr_t FLAG_MASK = 0x00000001;
const uintptr_t PTR_MASK = ~FLAG_MASK;

#define ATOMIC_PTR_AND_FLAG_MERGE(ptr, flag) (((uintptr_t) ptr) | (flag ? FLAG_MASK : 0))

/// Memory addresses of allocations on modern processors are (at least) 4 byte aligned,
/// this means that the lower 2 bytes of an allocated pointer are always 0. By using
/// some bit masking magic, it's possible to use the two lower bytes as storage for
/// flags. This class wraps an atomic pointer and allows the storage of a flag in the
/// lower unused bytes.
///
/// Note: It's possible to get pointers where the lower two bytes are used by indexing
/// into an allocation. If you always allocate a node using `malloc` or `new` this
/// will not be a problem here.
template<typename T>
class AtomicPtrAndFlag {
    std::atomic<intptr_t> ptr;

public:
    AtomicPtrAndFlag(T *ptr, bool flag) {
        this->ptr = ATOMIC_PTR_AND_FLAG_MERGE(ptr, flag);
    }

    /// Performs a compare-and-set operation. The value will only be
    /// updated if the current pointer and mark is equal to the ones
    /// given in the `test_*` arguments. The operation returns true
    /// if the comparison was successful and the value was updated.
    bool cas(T *test_ptr, T *new_ptr, bool test_mark, bool new_mark) {
        intptr_t test = ATOMIC_PTR_AND_FLAG_MERGE(test_ptr, test_mark);
        intptr_t set = ATOMIC_PTR_AND_FLAG_MERGE(new_ptr, new_mark);
        return this->ptr.compare_exchange_strong(test, set);
    }

    /// This tries to set the flag to `true`. It returns `true`,
    /// if the pointer matched and the value was update.
    bool try_set_mark(T *test_ptr) {
        return cas(test_ptr, test_ptr, false, true);
    }

    /// This returns the stored pointer and flag as a tuple. You can access
    /// the values using `std::tie` as shown below:
    ///
    /// ```c++
    /// LockFreeSetNode* ptr = nullptr;
    /// bool mark = false;
    /// std::tie (ptr, mark) = atomic.get();
    /// ```
    ///
    /// Or alternatively using `std::get`, like this:
    ///
    /// ```c++
    /// auto tuple = atomic.get()
    /// LockFreeSetNode* ptr = std::get<0>(tuple);
    /// bool mark = std::get<1>(tuple);
    /// ```
    std::tuple<T *, bool> get() {
        intptr_t value = this->ptr.load();
        T *ptr = (T *) (value & PTR_MASK);
        bool mark = (value & FLAG_MASK);

        return std::tuple<T *, bool>(ptr, mark);
    }

    /// This returns the stored pointer.
    ///
    /// Note: If you want to retrieve both the pointer and mark, you should use
    /// `AtomicPtrAndFlag::get()` to access both values atomically at once.
    T *get_ptr() {
        return std::get<0>(this->get());
    }

    /// This returns the flag pointer.
    ///
    /// Note: If you want to retrieve both the pointer and mark, you should use
    /// `AtomicPtrAndFlag::get()` to access both values atomically at once.
    bool get_flag() {
        return std::get<1>(this->get());
    }
};

struct LockFreeSetNode {
    // A02: You can add or remove fields as needed.
    int value;
    AtomicPtrAndFlag<LockFreeSetNode> next;

    LockFreeSetNode(int value, LockFreeSetNode *next) : value(value),
                                                        next(AtomicPtrAndFlag(next, false)) {
    }
};

struct LockFreeWindow {
    // A02: You can add or remove fields as needed.
    LockFreeSetNode *pred;
    LockFreeSetNode *curr;

    LockFreeWindow(LockFreeSetNode *pred, LockFreeSetNode *curr)
        : pred(pred), curr(curr) {
    }
};

/// A template for the implementation of a lock-free set, as shown in chapter
/// 9.8 in the course book. There this data structure is called `LockFreeList`.
class LockFreeSet : public Set {
private:
    // A02: You can add or remove fields as needed.
    LockFreeSetNode *head;

    LockFreeWindow find(int key) {
        LockFreeSetNode *pred;
        LockFreeSetNode *curr;
        LockFreeSetNode *succ;
        bool marked = false;
        bool snip = false;
    retry:
        while (true) {
            /// LockFreeSetNode* ptr = nullptr;
            /// bool mark = false;
            ///
            pred = head;
            curr = pred->next.get_ptr();
            while (true) {
                std::tie(succ, marked) = curr->next.get();
                if (marked) {
                    snip = pred->next.cas(curr, succ, false, false);
                    if (!snip) {
                        goto retry;
                    }
                    delete curr;
                    curr = succ;
                    std::tie(succ, marked) = curr->next.get();
                }
                if (curr->value >= key)
                    return LockFreeWindow(pred, curr);
                pred = curr;
                curr = succ;
            }
        }


        // A02: Implement the `find()` method as shown in figure 9.24 in the book
        //      Here you can use `this->head` instead of having the pointer as a
        //      function parameter.
    }

public:
    LockFreeSet() {
        // A02: Initialize the internal state.
        //      - The book doesn't specify how the state should be initialized.
        //        The code used in the book requires that there is always a valid
        //        head and tail pointer, since null chases are not specifically
        //        handled. The code below initializes a head and tail pointer.
        //        You're welcome to modify the code as needed for your implementation.
        LockFreeSetNode *tail = new LockFreeSetNode(INT_MAX, nullptr);
        head = new LockFreeSetNode(INT_MIN, tail);
    }

    ~LockFreeSet() {
        // A02: Cleanup any memory that was allocated
    }

    bool add(int value) override {
        // int key = item.hashCode();
        // while (true) {
        //     Window window = find(head, key);
        //     Node pred = window.pred, curr = window.curr;
        //     if (curr.key == key) {
        //         return false;
        //     } else {
        //         Node node = new Node(item);
        //         node.next = new AtomicMarkableReference(curr, false);
        //         if (pred.next.compareAndSet(curr, node, false, false)) {
        //             return true;
        //         }
        //     }
        // }
        while (true) {
            LockFreeWindow window = this->find(value);
            LockFreeSetNode *pred = window.pred;
            LockFreeSetNode *curr = window.curr;
            if (curr->value == value) {
                return false;
            } else {
                auto node = new LockFreeSetNode(value, curr);
                if (pred->next.cas(curr, node, false, false)) {
                    return true;
                }
            }
        }
    }

    bool rmv(int value) override {
        bool snip;
        while (true) {
            LockFreeWindow window = this->find(value);
            LockFreeSetNode *pred = window.pred;
            LockFreeSetNode *curr = window.curr;

            if (curr->value != value) {
                return false;
            }

            LockFreeSetNode *succ = curr->next.get_ptr();
            snip = curr->next.try_set_mark(succ);
            if (!snip)
                continue;
            pred->next.cas(curr, succ, false, false);
            return true;
        }
    }

    bool ctn(int value) override {
        bool marked = false;
        LockFreeSetNode *curr = head;
        while (curr->value < value) {
            std::tie(curr, marked) = curr->next.get();
        }
        return (curr->value == value && !marked);
    }

    void print_state() override {
        // A02: Optionally, add code to print the state. This is useful for debugging,
        // but not part of the assignment
        std::cout << "LockFreeSet {...}";
    }
};
