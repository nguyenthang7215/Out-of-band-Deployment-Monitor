#include "config.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

Config& Config::instance() {
    static Config instance;
    return instance;
}

bool Config::load_config(const std::string& config_path) {
    try {
        YAML::Node config = YAML::LoadFile(config_path);

        agent_id = config["agent_id"].as<std::string>();
        project_id = config["project_id"].as<std::string>();

        central_url = config["server"]["central_url"].as<std::string>();

        watch_paths.clear();
        for (const auto& path : config["watch_paths"]) {
            watch_paths.push_back(path.as<std::string>());
        }

        log_path = config["log"]["path"].as<std::string>();
        log_level = config["log"]["level"].as<std::string>();

        return true;

    } catch (const YAML::Exception& e) {
        std::cerr << "Không đọc được file YAML: " << e.what() << std::endl;
        return false;
    }
}

std::string Config::get_agent_id() const {
    return agent_id;
}

std::string Config::get_project_id() const {
    return project_id;
}

std::string Config::get_central_url() const {
    return central_url;
}

std::vector<std::string> Config::get_watch_paths() const {
    return watch_paths;
}

std::string Config::get_log_path() const {
    return log_path;
}

std::string Config::get_log_level() const {
    return log_level;
}