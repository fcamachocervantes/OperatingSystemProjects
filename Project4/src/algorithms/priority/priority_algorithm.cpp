#include "algorithms/priority/priority_algorithm.hpp"

#include <cassert>
#include <stdexcept>

#define FMT_HEADER_ONLY
#include "utilities/fmt/format.h"


/*
    Here is where you should define the logic for the priority algorithm.
*/

PRIORITYScheduler::PRIORITYScheduler(int slice) {
    if (slice != -1) {
        throw("PRIORITY must have a timeslice of -1");
    }
}

std::shared_ptr<SchedulingDecision> PRIORITYScheduler::get_next_thread() {
    if (readyQueue.empty()) {
        // No threads available
        auto decision = std::make_shared<SchedulingDecision>();
        decision->thread = nullptr;
        decision->explanation = "No threads available for scheduling.";
        return decision;
    }

    auto decision = std::make_shared<SchedulingDecision>();
    auto next_thread = this->readyQueue.top();
    int priority = next_thread->priority;
    readyQueue.pop();

    decision->explanation = "[S: " + std::to_string(threadCounts[0]) + " I: " + std::to_string(threadCounts[1]) + " N: " + std::to_string(threadCounts[2]) + " B: " + std::to_string(threadCounts[3]) + "] -> ";
    threadCounts[priority] -= 1;
    decision->explanation += "[S: " + std::to_string(threadCounts[0]) + " I: " + std::to_string(threadCounts[1]) + " N: " + std::to_string(threadCounts[2]) + " B: " + std::to_string(threadCounts[3]) + "]. Will run to completion of burst.";
    
    decision->thread = next_thread;

    return decision;
}

void PRIORITYScheduler::add_to_ready_queue(std::shared_ptr<Thread> thread) {
    this->readyQueue.push(thread->priority,thread);

    threadCounts[thread->priority] += 1;
}

size_t PRIORITYScheduler::size() const {
    return this->readyQueue.size();
}