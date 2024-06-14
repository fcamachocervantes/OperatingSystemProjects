#include "algorithms/rr/rr_algorithm.hpp"

#include <cassert>
#include <stdexcept>

#define FMT_HEADER_ONLY
#include "utilities/fmt/format.h"

/*
    Here is where you should define the logic for the round robin algorithm.
*/

RRScheduler::RRScheduler(int slice) {    
    if (slice <= 0) {
        this->time_slice = 3;
    } else {
        this->time_slice = slice;
    }
}

std::shared_ptr<SchedulingDecision> RRScheduler::get_next_thread() {
    auto decision = std::make_shared<SchedulingDecision>();

    if(readyQueue.empty()) {
        decision->thread = nullptr;
        decision->explanation = "No threads available for scheduling.";
        return decision;
    }

    decision->thread = readyQueue.front();
    decision->explanation = "Selected from " + std::to_string(readyQueue.size()) + " threads. Will run for at most " + std::to_string(this->time_slice) + " ticks.";
    decision->time_slice = this->time_slice;
    readyQueue.pop();

    return decision;
}

void RRScheduler::add_to_ready_queue(std::shared_ptr<Thread> thread) {
    readyQueue.push(thread);
}

size_t RRScheduler::size() const {
    return readyQueue.size();
}
