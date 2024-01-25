#include <thread>
#include <chrono>

class Clock {
public:
    void set_base_time(std::chrono::milliseconds _base_time);
    void start();
    void set_starting_time(std::chrono::high_resolution_clock::time_point time);
    void wait_until(std::chrono::milliseconds mark_time);
    std::chrono::milliseconds get_elapsed_time();

    std::chrono::milliseconds base_time;
    std::chrono::high_resolution_clock::time_point starting_time = {};


};