#include "helpers.h"
#include <libdqueue/q.h>
#include <libdqueue/queries.h>
#include <libdqueue/utils/utils.h>

#include <algorithm>

#include <catch.hpp>

using namespace dqueue;
using namespace dqueue::utils;
using namespace dqueue::queries;

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