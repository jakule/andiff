#include <gtest/gtest.h>

#include <synchronized_queue.hpp>

#include <thread>

TEST(SynchronizedQueueTest, BasicUsage) {
  synchronized_queue<int> q;
  EXPECT_TRUE(q.empty());

  int e0 = 5;
  q.push(e0);
  EXPECT_EQ(q.size(), 1);

  std::thread t1{[&q]() {
    int a;
    while (!q.wait_and_pop(a))
      ;
    EXPECT_EQ(a, 5);
    EXPECT_TRUE(q.wait_and_pop(a));
    EXPECT_EQ(a, 6);
  }};

  int e1 = 6;
  q.push(e1);

  q.close();
  EXPECT_TRUE(q.closed());
  t1.join();

  int e2;
  EXPECT_FALSE(q.wait_and_pop(e2));
}

TEST(SynchronizedQueueTest, MultipleThreads) {
  synchronized_queue<int> q;
  EXPECT_TRUE(q.empty());

  int e0 = 0, e1 = 1;
  q.push(e0);
  q.push(e1);
  EXPECT_EQ(q.size(), 2);

  std::vector<std::thread> v;
  for (int i = 0; i < 5; ++i) {
    v.emplace_back(std::move(std::thread{[&q]() {
      int a;
      while (q.wait_and_pop(a))
        ;

    }}));
  }

  q.close();
  EXPECT_TRUE(q.closed());

  for (auto& t : v) {
    t.join();
  }

  int e2;
  EXPECT_FALSE(q.wait_and_pop(e2));
}