#include <libdqueue/memory_message_pool.h>

#include "helpers.h"
#include <catch.hpp>

using namespace dqueue;

TEST_CASE("messagepool.memory") {
  queries::Publish pub1("q1", {1, 2, 3}, uint64_t(1));
  queries::Publish pub2("q2", {4, 5, 6}, uint64_t(2));

  MemoryMessagePool mp;
  mp.append(pub1);
  mp.append(pub2);
  EXPECT_EQ(mp.size(), size_t(2));
  auto all = mp.all();
  EXPECT_EQ(all.size(), size_t(2));

  mp.erase(pub1.messageId);
  all = mp.all();
  EXPECT_EQ(all.size(), size_t(1));
  EXPECT_EQ(all.front().messageId, pub2.messageId);
}
