#include "event_queue.h"

EventQueue::EventQueue(std::size_t max_size) {
    this->max_size = max_size;
}

bool EventQueue::push(MonitorEvent event) {
    std::unique_lock<std::mutex> lock(mtx);

    if (stopped) {
        return false;
    }

    if (queue.size() >= max_size) {
        return false; 
    }

    queue.push(std::move(event));
    
    cv.notify_one();
    return true;
}

std::optional<MonitorEvent> EventQueue::pop(int timeout_ms) {
    std::unique_lock<std::mutex> lock(mtx);

    if (cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this] { return !queue.empty() || stopped; })) {
        
        if (!queue.empty()) {
            MonitorEvent event = std::move(queue.front());
            queue.pop();
            return event;
        }
    }
    
    return std::nullopt;
}

void EventQueue::stop() {
    std::lock_guard<std::mutex> lock(mtx);
    stopped = true;
    cv.notify_all(); // Danh thuc tat ca thread dang cho pop()
}

std::size_t EventQueue::size() const {
    std::lock_guard<std::mutex> lock(mtx);
    return queue.size();
}

bool EventQueue::empty() const {
    std::lock_guard<std::mutex> lock(mtx);
    return queue.empty();
}

bool EventQueue::is_stopped() const {
    std::lock_guard<std::mutex> lock(mtx);
    return stopped;
}
