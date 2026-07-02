#include <gtest/gtest.h>
#include "audit_watcher.h"
#include "session_tracker.h"
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>

class AuditWatcherTest : public ::testing::Test {
protected:
    std::string test_log_file = "mock_audit.log";
    
    void SetUp() override {
        // Tao file log rong
        std::ofstream f(test_log_file);
        f.close();
    }
    
    void TearDown() override {
        std::remove(test_log_file.c_str());
    }
    
    void append_to_log(const std::string& content) {
        std::ofstream f(test_log_file, std::ios::app);
        f << content << "\n";
        f.close();
    }
};

TEST_F(AuditWatcherTest, ProcessCreateFileEvent) {
    SessionTracker tracker;
    AuditWatcher watcher(tracker);
    
    std::vector<AuditEvent> received_events;
    watcher.set_callback([&](const AuditEvent& e) {
        received_events.push_back(e);
    });
    
    // Start watcher (no se seek den cuoi file hien tai)
    watcher.start(test_log_file);
    
    // Doi 1 chut de watcher thread khoi dong xong
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Ghi log CREATE
    append_to_log("type=SYSCALL msg=audit(1620000000.000:100): arch=c000003e syscall=257 success=yes exit=3 a0=ffffff9c a1=7ffd3a0b3c60 a2=241 a3=1b6 items=2 ppid=1234 pid=1235 auid=1000 uid=1000 gid=1000 euid=1000 suid=1000 fsuid=1000 egid=1000 sgid=1000 fsgid=1000 tty=pts0 ses=123 comm=\"touch\" exe=\"/usr/bin/touch\" key=(null)");
    append_to_log("type=PATH msg=audit(1620000000.000:100): item=1 name=\"/opt/myapp/test.txt\" inode=123456 dev=08:01 mode=0100644 ouid=1000 ogid=1000 rdev=00:00 nametype=CREATE cap_fp=0 cap_fi=0 cap_fe=0 cap_fver=0 cap_frootid=0");
    append_to_log("type=PROCTITLE msg=audit(1620000000.000:100): proctitle=746F756368002F6F70742F6D796170702F746573742E747874");
    
    // Can co mot event khac de flush event truoc do (do co che so sanh audit_serial)
    append_to_log("type=SYSCALL msg=audit(1620000000.001:101): arch=c000003e syscall=2 success=yes");
    
    // Doi watcher doc
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    watcher.stop();
    
    ASSERT_EQ(received_events.size(), 1);
    EXPECT_EQ(received_events[0].file_path, "/opt/myapp/test.txt");
    EXPECT_EQ(received_events[0].file_name, "test.txt");
    EXPECT_EQ(received_events[0].event_type, EventType::CREATE);
    EXPECT_EQ(received_events[0].exe_path, "/usr/bin/touch");
    EXPECT_EQ(received_events[0].session_id, 123);
}

TEST_F(AuditWatcherTest, ProcessDeleteFileEvent) {
    SessionTracker tracker;
    AuditWatcher watcher(tracker);
    
    std::vector<AuditEvent> received_events;
    watcher.set_callback([&](const AuditEvent& e) {
        received_events.push_back(e);
    });
    
    watcher.start(test_log_file);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Ghi log DELETE
    append_to_log("type=SYSCALL msg=audit(1620000001.000:102): arch=c000003e syscall=263 success=yes exit=0 a0=ffffff9c a1=7ffe1b8b8b80 a2=0 a3=7ffe1b8b7a70 items=2 ppid=1234 pid=1236 auid=1000 uid=1000 gid=1000 euid=1000 suid=1000 fsuid=1000 egid=1000 sgid=1000 fsgid=1000 tty=pts0 ses=123 comm=\"rm\" exe=\"/usr/bin/rm\" key=(null)");
    append_to_log("type=PATH msg=audit(1620000001.000:102): item=1 name=\"/opt/myapp/test.txt\" inode=123456 dev=08:01 mode=0100644 ouid=1000 ogid=1000 rdev=00:00 nametype=DELETE cap_fp=0 cap_fi=0 cap_fe=0 cap_fver=0 cap_frootid=0");
    append_to_log("type=PROCTITLE msg=audit(1620000001.000:102): proctitle=726D002F6F70742F6D796170702F746573742E747874");
    
    // Flush event
    append_to_log("type=SYSCALL msg=audit(1620000001.001:103): arch=c000003e syscall=2 success=yes");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    watcher.stop();
    
    ASSERT_EQ(received_events.size(), 1);
    EXPECT_EQ(received_events[0].file_path, "/opt/myapp/test.txt");
    EXPECT_EQ(received_events[0].event_type, EventType::DELETE);
    EXPECT_EQ(received_events[0].exe_path, "/usr/bin/rm");
}
