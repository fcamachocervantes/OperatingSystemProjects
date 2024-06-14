#include "algorithms/fcfs/fcfs_algorithm.hpp"

#include <cassert>
#include <stdexcept>

#define FMT_HEADER_ONLY
#include "utilities/fmt/format.h"

/*
    Here is where you should define the logic for the FCFS algorithm.
*/

FCFSScheduler::FCFSScheduler(int slice) {
    if (slice != -1) {
        throw("FCFS must have a timeslice of -1");
    }
}

std::shared_ptr<SchedulingDecision> FCFSScheduler::get_next_thread() {
    // No threads are available
    if (readyQueue.empty()) {
        auto decision = std::make_shared<SchedulingDecision>();
        decision->thread = nullptr;
        decision->explanation = "No threads available for scheduling.";
        return decision;
    }

    // Get the next thread in FCFS order
    std::shared_ptr<Thread> nextThread = readyQueue.front();
    readyQueue.pop();

    // Create a scheduling decision object and return it
    auto decision = std::make_shared<SchedulingDecision>();
    decision->thread = nextThread;
    decision->explanation = "Selected from " + std::to_string(readyQueue.size() + 1) + " threads. Will run to completion of burst.";

    // FCFS runs a process until completion so no time slice
    decision->time_slice = -1;
    return decision;
}

void FCFSScheduler::add_to_ready_queue(std::shared_ptr<Thread> thread) {
    readyQueue.push(thread);
}

size_t FCFSScheduler::size() const {
    return readyQueue.size();
}
