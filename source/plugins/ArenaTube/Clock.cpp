#include "Clock.h"

void Clock::set_base_time(std::chrono::milliseconds _base_time) {
    base_time = _base_time;
}

void Clock::start() {
    set_starting_time(std::chrono::high_resolution_clock::now());
}

void Clock::set_starting_time(std::chrono::high_resolution_clock::time_point time) {
    starting_time = time;
}

std::chrono::milliseconds Clock::get_elapsed_time() {
    return std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::high_resolution_clock::now() - starting_time);
}

void Clock::wait_until(std::chrono::milliseconds mark_time)
{
    std::chrono::milliseconds current_time = base_time + get_elapsed_time();

    printf("current %d, base %d, elapsed %d, mark_time %d\n", current_time, base_time, get_elapsed_time(), mark_time);

    if (current_time < mark_time) {
        std::this_thread::sleep_for(std::chrono::milliseconds(mark_time - current_time));
    }
}