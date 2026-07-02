#include <gtest/gtest.h>
#include "event_queue.h"
#include <thread>
#include <chrono>

TEST(EventQueueTest, BasicPushPop) {
    EventQueue queue(10);
    MonitorEvent e1; e1.event_id = "test-1";
    
    EXPECT_TRUE(queue.push(e1));
    EXPECT_EQ(queue.size(), 1);
    EXPECT_FALSE(queue.empty());

    auto result = queue.pop(100);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->event_id, "test-1");
    EXPECT_TRUE(queue.empty());
}

TEST(EventQueueTest, PopTimeoutWhenEmpty) {
    EventQueue queue(10);
    auto start = std::chrono::steady_clock::now();
    
    auto result = queue.pop(50); // Cho 50ms
    
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_FALSE(result.has_value());
    EXPECT_GE(elapsed, 50); // Phai cho it nhat 50ms
}

TEST(EventQueueTest, DropWhenFull) {
    EventQueue queue(2); // max 2
    MonitorEvent e;
    
    EXPECT_TRUE(queue.push(e));
    EXPECT_TRUE(queue.push(e));
    EXPECT_FALSE(queue.push(e)); // Cai thu 3 phai bi drop
    
    EXPECT_EQ(queue.size(), 2);
}

TEST(EventQueueTest, ThreadSafety) {
    EventQueue queue(1000);
    
    auto producer = [&]() {
        for(int i = 0; i < 500; ++i) {
            MonitorEvent e; e.event_id = "th-" + std::to_string(i);
            queue.push(e);
        }
    };

    auto consumer = [&]() {
        int count = 0;
        while(count < 500) {
            auto res = queue.pop(10);
            if(res) count++;
        }
    };

    std::thread t1(producer);
    std::thread t2(consumer);
    
    t1.join();
    t2.join();
    
    EXPECT_TRUE(queue.empty());
}

TEST(EventQueueTest, StopAwakesWaitingThread) {
    EventQueue queue(10);
    
    bool pop_returned = false;
    std::thread t1([&]() {
        // Cho 10 giay, nhung se bi danh thuc boi stop()
        auto res = queue.pop(10000); 
        pop_returned = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    queue.stop(); // Danh thuc
    
    t1.join();
    EXPECT_TRUE(pop_returned);
    EXPECT_TRUE(queue.is_stopped());
}
