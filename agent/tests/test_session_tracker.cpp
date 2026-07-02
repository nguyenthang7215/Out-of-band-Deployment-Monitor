#include <gtest/gtest.h>
#include "session_tracker.h"

TEST(SessionTrackerTest, ValidSSHLogin) {
    SessionTracker tracker;
    std::string line = "type=USER_LOGIN msg=audit(1620000000.000:100): pid=123 uid=0 auid=1000 ses=123 msg='op=login acct=\"ubuntu\" exe=\"/usr/sbin/sshd\" hostname=? addr=192.168.1.5 terminal=ssh res=success'";
    
    tracker.process_line(line);
    
    EXPECT_EQ(tracker.get_source_ip(123), "192.168.1.5");
}

TEST(SessionTrackerTest, FailedLogin) {
    SessionTracker tracker;
    std::string line = "type=USER_LOGIN msg=audit(1620000000.000:101): pid=124 uid=0 auid=1000 ses=124 msg='op=login acct=\"ubuntu\" exe=\"/usr/sbin/sshd\" hostname=? addr=192.168.1.6 terminal=ssh res=failed'";
    
    tracker.process_line(line);
    
    // Vi login failed, IP khong duoc luu lai vao cache
    EXPECT_EQ(tracker.get_source_ip(124), "127.0.0.1"); // Mac dinh tra ve local
}

TEST(SessionTrackerTest, LocalhostLogin) {
    SessionTracker tracker;
    std::string line1 = "type=USER_LOGIN msg=audit(1620000000.000:102): pid=125 uid=0 auid=1000 ses=125 msg='op=login acct=\"ubuntu\" exe=\"/usr/sbin/sshd\" hostname=? addr=? terminal=ssh res=success'";
    std::string line2 = "type=USER_LOGIN msg=audit(1620000000.000:103): pid=126 uid=0 auid=1000 ses=126 msg='op=login acct=\"ubuntu\" exe=\"/usr/sbin/sshd\" hostname=? addr=127.0.0.1 terminal=ssh res=success'";
    
    tracker.process_line(line1);
    tracker.process_line(line2);
    
    // IP local hoac '?' deu khong hop le, khong luu vao cache
    EXPECT_EQ(tracker.get_source_ip(125), "127.0.0.1");
    EXPECT_EQ(tracker.get_source_ip(126), "127.0.0.1");
}

TEST(SessionTrackerTest, BackgroundProcessLogin) {
    SessionTracker tracker;
    std::string line = "type=USER_LOGIN msg=audit(1620000000.000:104): pid=127 uid=0 auid=4294967295 ses=4294967295 msg='op=login acct=\"root\" exe=\"/usr/sbin/cron\" hostname=? addr=? terminal=cron res=success'";
    
    tracker.process_line(line);
    
    // Ses 4294967295 la tien trinh he thong, se bi bo qua ngay tu dau
    EXPECT_EQ(tracker.get_source_ip(-1), "127.0.0.1");
}

TEST(SessionTrackerTest, MissingFields) {
    SessionTracker tracker;
    // Thieu res
    std::string line1 = "type=USER_LOGIN msg=audit(1620000000.000:100): pid=123 uid=0 auid=1000 ses=128 msg='op=login addr=192.168.1.5'";
    // Thieu addr
    std::string line2 = "type=USER_LOGIN msg=audit(1620000000.000:100): pid=123 uid=0 auid=1000 ses=129 msg='op=login res=success'";
    // Thieu ses
    std::string line3 = "type=USER_LOGIN msg=audit(1620000000.000:100): pid=123 uid=0 auid=1000 msg='op=login addr=192.168.1.5 res=success'";

    tracker.process_line(line1);
    tracker.process_line(line2);
    tracker.process_line(line3);

    EXPECT_EQ(tracker.get_source_ip(128), "127.0.0.1");
    EXPECT_EQ(tracker.get_source_ip(129), "127.0.0.1");
}
