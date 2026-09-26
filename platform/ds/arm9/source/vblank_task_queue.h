#pragma once

// Fixed storage for callbacks requested from both the foreground and VBlank.
// The caller must exclude VBlank while enqueueing from the foreground.
template <unsigned Capacity>
class VBlankTaskQueue {
public:
    typedef void (*Task)();
    struct Batch {
        const Task* tasks;
        unsigned count;
    };

    VBlankTaskQueue() : pending_(0) {
        lengths_[0] = 0;
        lengths_[1] = 0;
    }

    bool enqueue(Task task) {
        if (!task || lengths_[pending_] == Capacity)
            return false;
        tasks_[pending_][lengths_[pending_]++] = task;
        return true;
    }

    // Switch before invoking callbacks: a callback can enqueue for the next
    // VBlank without changing this batch or allocating inside the IRQ.
    Batch beginDrain() {
        const unsigned ready = pending_;
        pending_ ^= 1;
        Batch batch = {tasks_[ready], lengths_[ready]};
        lengths_[ready] = 0;
        return batch;
    }

private:
    Task tasks_[2][Capacity];
    unsigned lengths_[2];
    unsigned pending_;
};
