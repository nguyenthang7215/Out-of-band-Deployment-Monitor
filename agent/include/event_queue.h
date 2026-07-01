#pragma once

#include "event.h"
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>

class EventQueue {
public:
    explicit EventQueue(std::size_t max_size = 1000);

    bool push(MonitorEvent event);
    std::optional<MonitorEvent> pop(int timeout_ms); // blocking
    void stop();

    std::size_t size() const;
    bool empty() const;
    bool is_stopped() const;

private:
    std::queue<MonitorEvent> queue;
    mutable std::mutex mtx;
    std::condition_variable cv;
    std::size_t max_size;
    bool stopped = false;
};