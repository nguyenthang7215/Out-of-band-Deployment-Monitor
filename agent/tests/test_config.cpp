#include <gtest/gtest.h>
#include "config.h"
#include <fstream>
#include <cstdio>

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Khong lam gi truoc moi test
    }

    void TearDown() override {
        // Xoa cac file config tam thoi neu co
        std::remove("valid_config.yaml");
        std::remove("invalid_config.yaml");
    }

    void create_yaml_file(const std::string& filename, const std::string& content) {
        std::ofstream file(filename);
        file << content;
        file.close();
    }
};

// Test 1: Load file YAML hop le
TEST_F(ConfigTest, LoadValidConfig) {
    std::string yaml_content = R"(
agent_id: "agent-test"
project_id: "project-test"
server:
  central_url: "http://localhost:8080/api/v1/events"
watch_paths:
  - "/opt/myapp"
  - "/etc"
log:
  path: "/var/log/vdt-agent.log"
  level: "DEBUG"
)";
    create_yaml_file("valid_config.yaml", yaml_content);

    // Vi Config la Singleton, nen phai luu y no giu trang thai
    // Tuy nhien ham load_config se ghi de cac bien ben trong.
    bool success = Config::instance().load_config("valid_config.yaml");
    
    EXPECT_TRUE(success);
    EXPECT_EQ(Config::instance().get_agent_id(), "agent-test");
    EXPECT_EQ(Config::instance().get_project_id(), "project-test");
    EXPECT_EQ(Config::instance().get_central_url(), "http://localhost:8080/api/v1/events");
    EXPECT_EQ(Config::instance().get_log_level(), "DEBUG");
    
    auto paths = Config::instance().get_watch_paths();
    ASSERT_EQ(paths.size(), 2);
    EXPECT_EQ(paths[0], "/opt/myapp");
    EXPECT_EQ(paths[1], "/etc");
}

// Test 2: Thieu truong bat buoc
TEST_F(ConfigTest, LoadMissingRequiredFields) {
    std::string yaml_content = R"(
agent_id: "agent-test"
project_id: "project-test"
# Thieu server.central_url
)";
    create_yaml_file("invalid_config.yaml", yaml_content);

    bool success = Config::instance().load_config("invalid_config.yaml");
    
    // Theo yeu cau: "test khi thiếu field bắt buộc trả false"
    EXPECT_FALSE(success);
}
