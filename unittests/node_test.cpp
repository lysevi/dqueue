#include "helpers.h"
#include <libdqueue/node.h>
#include <catch.hpp>
#include <set>
using namespace dqueue;
using namespace dqueue::utils;

TEST_CASE("node.queue_subscription") {
  Node::Settings settings;
  std::string tag = "tag1";
  std::set<Id> sends;

  DataHandler dhandler = [&sends, tag](const PublishParams &info, const std::vector<uint8_t> &, Id id) {
    EXPECT_TRUE(!info.queueName.empty());
    if (info.queueName == "q2" && id == 1) {
      EXPECT_EQ(info.tag, tag);
    }
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
  n.createQueue(QueueSettings("q3"), 3); //owner must be still 2

  auto descr = n.getQueuesDescription();
  EXPECT_EQ(descr.size(), size_t(3));

  n.changeSubscription(SubscribeActions::Subscribe, SubscriptionParams("q1"), 1);
  n.changeSubscription(SubscribeActions::Subscribe, SubscriptionParams("q2", tag), 1);
  n.changeSubscription(SubscribeActions::Subscribe, SubscriptionParams("q3"), 1);

  n.changeSubscription(SubscribeActions::Subscribe, SubscriptionParams("q3"), 2);
  n.changeSubscription(SubscribeActions::Subscribe, SubscriptionParams("q2", "tag2"), 2);

  n.changeSubscription(SubscribeActions::Subscribe, SubscriptionParams("q3"), 3);
  n.changeSubscription(SubscribeActions::Unsubscribe, SubscriptionParams("q3"), 1);

  descr = n.getQueuesDescription();
  for (auto d : descr) {
    if (d.settings.name == "q1") {
      EXPECT_EQ(d.subscribers.size(), size_t(1));
    }
    if (d.settings.name == "q2") {
      EXPECT_EQ(d.subscribers.size(), size_t(2));
    }
    if (d.settings.name == "q3") {
      EXPECT_EQ(d.subscribers.size(), size_t(2));
    }
  }

  n.publish(PublishParams("q1", tag), std::vector<uint8_t>(1), 2); //author must be not in result
  EXPECT_EQ(sends.size(), size_t(1));
  EXPECT_TRUE(std::find(sends.begin(), sends.end(), 1) != sends.end());

  sends.clear();

  n.publish(PublishParams("q2", tag), std::vector<uint8_t>(1), 3);
  EXPECT_EQ(sends.size(), size_t(1));
  EXPECT_TRUE(std::find(sends.begin(), sends.end(), 1) != sends.end());

  sends.clear();

  n.publish(PublishParams("q3"), std::vector<uint8_t>(3), 1);
  EXPECT_EQ(sends.size(), size_t(2));
  EXPECT_TRUE(std::find(sends.begin(), sends.end(), 1) == sends.end());
  EXPECT_TRUE(std::find(sends.begin(), sends.end(), 2) != sends.end());
  EXPECT_TRUE(std::find(sends.begin(), sends.end(), 3) != sends.end());
}
