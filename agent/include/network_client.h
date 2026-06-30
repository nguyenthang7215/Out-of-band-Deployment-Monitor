#pragma once 

#include <string>

class NetworkClient { 
public: 
    static NetworkClient& instance();
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;
    ~NetworkClient();

    void init(const std::string& central_url);
    bool send_event(const std::string& json_data);
    bool send_event_with_retry(const std::string& json_data, int max_retries = 3);

private: 
    NetworkClient() = default;
    std::string central_url;
};