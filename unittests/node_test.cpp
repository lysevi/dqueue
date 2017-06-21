#include "helpers.h"
#include <libdqueue/node.h>
#include <catch.hpp>
#include <set>
using namespace dqueue;
using namespace dqueue::utils;

TEST_CASE("node.queue_subscription") {
  Node::Settings settings;
  std::set<int> sends;

  DataHandler dhandler = [&sends](const std::string &queueName,
                                        const rawData &, int id) {
    EXPECT_TRUE(!queueName.empty());
    sends.insert(id);
  };

  Node n(settings, dhandler);

  Node::Client cl1{1};
  Node::Client cl2{2};
  Node::Client cl3{3};
  n.addClient(cl1);
  n.addClient(cl2);
  n.addClient(cl3);
  auto cldescr = n.getClientsDescription();
  EXPECT_EQ(cldescr.size(), size_t(3));

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

  cldescr = n.getClientsDescription();
  EXPECT_EQ(cldescr.size(), size_t(3));
  for (auto cd : cldescr) {
    if (cd.id == 1) {
      EXPECT_EQ(cd.subscribtions.size(), size_t(2));
      EXPECT_TRUE(std::find(cd.subscribtions.begin(), cd.subscribtions.end(), "q1") !=
                  cd.subscribtions.end());

      EXPECT_TRUE(std::find(cd.subscribtions.begin(), cd.subscribtions.end(), "q2") !=
                  cd.subscribtions.end());

      EXPECT_TRUE(std::find(cd.subscribtions.begin(), cd.subscribtions.end(), "q3") ==
                  cd.subscribtions.end());
    }
    if (cd.id == 2) {
      EXPECT_EQ(cd.subscribtions.size(), size_t(1));
      EXPECT_TRUE(std::find(cd.subscribtions.begin(), cd.subscribtions.end(), "q3") !=
                  cd.subscribtions.end());
    }
    if (cd.id == 3) {
      EXPECT_EQ(cd.subscribtions.size(), size_t(1));
    }
  }

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

  n.publish("q1", rawData(1));
  EXPECT_EQ(sends.size(), size_t(1));
  EXPECT_TRUE(std::find(sends.begin(), sends.end(), 1) != sends.end());

  sends.clear();

  n.publish("q3", rawData(3));
  EXPECT_EQ(sends.size(), size_t(2));
  EXPECT_TRUE(std::find(sends.begin(), sends.end(), 1) == sends.end());
  EXPECT_TRUE(std::find(sends.begin(), sends.end(), 2) != sends.end());
  EXPECT_TRUE(std::find(sends.begin(), sends.end(), 3) != sends.end());
}
