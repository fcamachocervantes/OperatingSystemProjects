#include "algorithms/spn/spn_algorithm.hpp"

#include <cassert>
#include <stdexcept>

#define FMT_HEADER_ONLY
#include "utilities/fmt/format.h"

/*
    Here is where you should define the logic for the SPN algorithm.
*/

SPNScheduler::SPNScheduler(int slice) {
    if (slice != -1) {
        throw std::invalid_argument("SPN must have a timeslice of -1");
    }
}

std::shared_ptr<SchedulingDecision> SPNScheduler::get_next_thread() {
    if (readyQueue.empty()) {
        // No threads available
        auto decision = std::make_shared<SchedulingDecision>();
        decision->thread = nullptr;
        decision->explanation = "No threads available for scheduling.";
        return decision;
    }

    // Get the next thread with the shortest burst time
    auto shortest_thread = readyQueue.top();
    readyQueue.pop();

    // Create a scheduling decision object and return it
    auto decision = std::make_shared<SchedulingDecision>();
    decision->thread = shortest_thread;
    decision->explanation = "Selected from " + std::to_string(readyQueue.size() + 1) + " threads. Will run to completion of burst.";

    // SPN runs a process until completion so no time slice
    decision->time_slice = -1;
    return decision;
}

void SPNScheduler::add_to_ready_queue(std::shared_ptr<Thread> thread) {  
    // Get the cpu burst time of the thread
    auto cpu_burst_time = thread->get_next_burst(BurstType::CPU)->length;

    // Add the thread to the ready queue with the burst time as priority
    readyQueue.push(cpu_burst_time, thread);
}

size_t SPNScheduler::size() const {
    return readyQueue.size();
}
