#pragma once 

#include <string>
#include <thread>
#include <atomic>
#include "event_queue.h"

class NetworkClient { 
public: 
    NetworkClient(const std::string& central_url, EventQueue& queue);
    ~NetworkClient();

    void start();
    void stop();

private: 
    bool send_event_with_retry(const std::string& json_data, int max_retries = 3);
    bool send_event(const std::string& json_data);
    void sender_loop();

    std::string central_url;
    EventQueue& event_queue;
    std::thread sender_thread;
    std::atomic<bool> running{false};
};