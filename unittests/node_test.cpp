#include "helpers.h"
#include <libdqueue/node.h>
#include <catch.hpp>
#include <set>
using namespace dqueue;
using namespace dqueue::utils;

TEST_CASE("node.queue_subscription") {
  Node::Settings settings;
  std::set<Id> sends;

  DataHandler dhandler = [&sends](const MessageInfo&info, const rawData &, Id id) {
    EXPECT_TRUE(!info.queueName.empty());
    sends.insert(id);
  };

  auto ub = UserBase::create();

  Node n(settings, dhandler, ub);

  User cl1{"1", 1};
  User cl2{"2", 2};
  User cl3{"3", 3};
  ub->append(cl1);
  ub->append(cl2);
  ub->append(cl3);

  n.createQueue(QueueSettings("q1"), 1);
  n.createQueue(QueueSettings("q2"), 1);
  n.createQueue(QueueSettings("q3"), 2);
  auto descr = n.getQueuesDescription();
  EXPECT_EQ(descr.size(), size_t(3));

  n.changeSubscription(Node::SubscribeActions::Subscribe, "q1", 1);
  n.changeSubscription(Node::SubscribeActions::Subscribe, "q2", 1);
  n.changeSubscription(Node::SubscribeActions::Subscribe, "q3", 1);
  n.changeSubscription(Node::SubscribeActions::Subscribe, "q3", 2);
  n.changeSubscription(Node::SubscribeActions::Subscribe, "q3", 3);
  n.changeSubscription(Node::SubscribeActions::Unsubscribe, "q3", 1);

  descr = n.getQueuesDescription();
  for (auto d : descr) {
    if (d.settings.name == "q1") {
      EXPECT_EQ(d.subscribers.size(), size_t(1));
    }
    if (d.settings.name == "q2") {
      EXPECT_EQ(d.subscribers.size(), size_t(1));
    }
    if (d.settings.name == "q3") {
      EXPECT_EQ(d.subscribers.size(), size_t(2));
    }
  }

  n.publish("q1", rawData(1), 2);
  EXPECT_EQ(sends.size(), size_t(1));
  EXPECT_TRUE(std::find(sends.begin(), sends.end(), 1) != sends.end());

  sends.clear();

  n.publish("q3", rawData(3), 1);
  EXPECT_EQ(sends.size(), size_t(2));
  EXPECT_TRUE(std::find(sends.begin(), sends.end(), 1) == sends.end());
  EXPECT_TRUE(std::find(sends.begin(), sends.end(), 2) != sends.end());
  EXPECT_TRUE(std::find(sends.begin(), sends.end(), 3) != sends.end());
}
