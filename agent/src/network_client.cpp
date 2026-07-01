#include "network_client.h"
#include "logger.h"
#include "event_serializer.h"
#include <curl/curl.h>
#include <thread>
#include <chrono>

NetworkClient::NetworkClient(const std::string& central_url, EventQueue& queue)
    : central_url(central_url), event_queue(queue) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    LOG_INFO("NetworkClient đã khởi tạo, kết nối tới: " + this->central_url);
}

NetworkClient::~NetworkClient() {
    stop();
    curl_global_cleanup();
    LOG_INFO("NetworkClient đã dọn dẹp tài nguyên");
}

void NetworkClient::start() {
    if (running) return;
    running = true;
    sender_thread = std::thread(&NetworkClient::sender_loop, this);
}

void NetworkClient::stop() {
    if (running) {
        running = false;
        if (sender_thread.joinable()) {
            sender_thread.join();
        }
    }
}

void NetworkClient::sender_loop() {
    while (running) {
        auto event_opt = event_queue.pop(500);
        if (event_opt) {
            std::string json_data = EventSerializer::to_json(*event_opt);
            if (!send_event_with_retry(json_data)) {
                LOG_ERROR("Gửi event thất bại: " + event_opt->event_id);
            } else {
                LOG_INFO("Gửi event thành công: " + event_opt->event_id);
            }
        }
    }
}

bool NetworkClient::send_event(const std::string& json_data) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Không thể khởi tạo CURL để gửi sự kiện");
        return false;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, this->central_url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    // Tranh in ra response khong can thiet
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void*, size_t size, size_t nmemb, void*) -> size_t {
        return size * nmemb; 
    });

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    bool success = (res == CURLE_OK) && (http_code >= 200 && http_code < 300);

    if (!success) {
        if (res != CURLE_OK) {
            LOG_ERROR("Gửi cảnh báo thất bại (Lỗi mạng): " + std::string(curl_easy_strerror(res)));
        } else {
            LOG_ERROR("Gửi cảnh báo thất bại (Lỗi HTTP Server: " + std::to_string(http_code) + ")");
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return success;
}

bool NetworkClient::send_event_with_retry(const std::string& json_data, int max_retries) {
    int attempt = 0;
    int delay_ms = 500;

    while (attempt < max_retries && running) { // Thoat som neu agent dang stop
        if (send_event(json_data)) {
            return true;
        }
        
        attempt++;
        if (attempt < max_retries && running) {
            LOG_WARN("Thử lại lần " + std::to_string(attempt + 1) + " sau " + std::to_string(delay_ms) + "ms");
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            delay_ms *= 2;
        }
    }
    
    if (running) { // Log loi neu that bai do mang, khong phai do agent tat ngang
        LOG_ERROR("Gửi cảnh báo thất bại hoàn toàn sau " + std::to_string(max_retries) + " lần thử");
    }
    return false;
}
