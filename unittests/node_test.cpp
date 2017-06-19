#include "helpers.h"
#include <libdqueue/node.h>
#include <catch.hpp>

using namespace dqueue;
using namespace dqueue::utils;

TEST_CASE("node.queue_settings") {
	auto name = "node.queue_settings.name";
	
	QueueSettings qs(name);
	EXPECT_EQ(qs.name, name);

	auto nd=qs.toNetworkMessage();
	EXPECT_EQ(nd->cast_to_header()->kind, 0);

	auto repacked=QueueSettings::fromNetworkMessage(nd);
	EXPECT_EQ(repacked.name, name);
}

TEST_CASE("node.create_queue") {
  Node::Settings settings;
  Node n(settings);
  n.createQueue(QueueSettings("q1"));
  n.createQueue(QueueSettings("q2"));
  n.createQueue(QueueSettings("q3"));
  auto descr = n.getDescription();
  EXPECT_EQ(descr.size(), size_t(3));
}
