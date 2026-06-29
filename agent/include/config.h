#pragma once

#include <string>
#include <vector> 

class Config {
public: 
    static Config& instance();
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    ~Config() = default;

    bool load_config(const std::string& config_path);

    std::string get_agent_id() const;
    std::string get_project_id() const;
    std::string get_central_url() const;
    std::vector<std::string> get_watch_paths() const;
    std::string get_log_path() const;
    std::string get_log_level() const;
 
private: 
    Config() = default;
    std::string agent_id;
    std::string project_id;
    std::string central_url;
    std::vector<std::string> watch_paths; // Danh sach cac thu muc can giam sat
    std::string log_path;
    std::string log_level;
};