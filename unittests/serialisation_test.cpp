#include "helpers.h"
#include <libdqueue/q.h>
#include <libdqueue/queries.h>
#include <libdqueue/serialisation.h>
#include <libdqueue/utils/utils.h>
#include <algorithm>

#include <catch.hpp>

using namespace dqueue;
using namespace dqueue::utils;
using namespace dqueue::queries;

TEST_CASE("serialisation.size_of_args") {
  EXPECT_EQ(serialisation::Scheme<int>::size_of_args(int(1)), sizeof(int));
  auto sz = serialisation::Scheme<int,int>::size_of_args(int(1), int(1));
  EXPECT_EQ(sz, sizeof(int) * 2);

  sz = serialisation::Scheme<int, int, double>::size_of_args(int(1), int(1), double(1.0));
  EXPECT_EQ(sz, sizeof(int) * 2 + sizeof(double));

  std::string str = "hello world";
  sz = serialisation::Scheme<std::string>::size_of_args(std::move(str));
  EXPECT_EQ(sz, sizeof(uint32_t) + str.size());
}

TEST_CASE("serialisation.scheme") {
  EXPECT_EQ(serialisation::Scheme<int>(1).buffer.size(), sizeof(int));
  serialisation::Scheme<int, int> i2(1, 2);
  EXPECT_EQ(i2.buffer.size(), sizeof(int) * 2);

  int unpacked1, unpacked2;
  serialisation::Scheme<int, int> i2_cpy;
  i2_cpy.init_from_buffer(i2.buffer);
  i2_cpy.readTo(unpacked1, unpacked2);
  EXPECT_EQ(unpacked1, 1);
  EXPECT_EQ(unpacked2, 2);

  std::string str = "hello world";
  serialisation::Scheme<int, std::string> s1(11, std::move(str));
  EXPECT_EQ(s1.buffer.size(), sizeof(int) + sizeof(uint32_t) + str.size());
  std::string unpackedS;
  s1.readTo(unpacked1, unpackedS);
  EXPECT_EQ(unpacked1, 11);
  EXPECT_EQ(unpackedS, str);
}

TEST_CASE("serialisation.queue_settings") {
  std::string name = "node.queue_settings.name";

  QueueSettings qs(name);
  EXPECT_EQ(qs.name, name);

  auto nd = qs.toNetworkMessage();
  EXPECT_EQ(nd->cast_to_header()->kind, 0);

  auto repacked = QueueSettings::fromNetworkMessage(nd);
  EXPECT_EQ(repacked.name.size(), name.size());
  EXPECT_EQ(repacked.name, name);
}

TEST_CASE("serialisation.subscribe") {
  std::string qname = "serialisation.subscribe";
  ChangeSubscribe cs{qname};
  auto nd = cs.toNetworkMessage();
  EXPECT_EQ(nd->cast_to_header()->kind,
            (NetworkMessage::message_kind)MessageKinds::SUBSCRIBE);

  auto repacked = ChangeSubscribe(nd);
  EXPECT_EQ(repacked.qname, qname);
  EXPECT_EQ(repacked.qname.size(), qname.size());
}

TEST_CASE("serialisation.publish") {
  std::string qname = "serialisation.publish";
  std::vector<uint8_t> data = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  Publish pb{qname, data};
  auto nd = pb.toNetworkMessage();
  EXPECT_EQ(nd->cast_to_header()->kind,
            (NetworkMessage::message_kind)MessageKinds::PUBLISH);

  auto repacked = Publish(nd);
  EXPECT_EQ(repacked.qname, qname);
  EXPECT_EQ(repacked.qname.size(), qname.size());

  EXPECT_EQ(repacked.data.size(), data.size());
  EXPECT_TRUE(
      std::equal(data.begin(), data.end(), repacked.data.begin(), repacked.data.end()));
}