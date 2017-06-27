#include "helpers.h"
#include <libdqueue/client.h>
#include <libdqueue/server.h>
#include <libdqueue/utils/logger.h>
#include <catch.hpp>

#include <boost/asio.hpp>

#include <functional>
#include <string>
#include <thread>

using namespace std::placeholders;
using namespace boost::asio;

using namespace dqueue;
using namespace dqueue::utils;

namespace server_client_test {

bool server_stop = false;
std::shared_ptr<Server> server = nullptr;
boost::asio::io_service *service;
void server_thread() {
  AbstractServer::params p;
  p.port = 4040;
  service = new boost::asio::io_service();
  server = std::make_shared<Server>(service, p);

  server->serverStart();
  while (!server_stop) {
    service->poll_one();
  }

  server->stopServer();
  service->stop();
  EXPECT_TRUE(service->stopped());
  delete service;
  server = nullptr;
}

void testForReconnection(const size_t clients_count) {
  AbstractClient::Params p("empty", "localhost", 4040);

  server_stop = false;
  std::thread t(server_thread);
  while (server == nullptr || !server->is_started()) {
    logger("server.client.testForReconnection. !server->is_started serverIsNull? ",
           server == nullptr);
  }

  std::vector<std::shared_ptr<Client>> clients(clients_count);
  for (size_t i = 0; i < clients_count; i++) {
    p.login = "client_" + std::to_string(i);
    clients[i] = std::make_shared<Client>(service, p);
    clients[i]->connectAsync();
  }

  for (auto &c : clients) {
    while (!c->is_connected()) {
      logger("server.client.testForReconnection. client not connected");
    }
  }

  while (true) {
    auto users = server->users();
    bool loginned = false;
    for (auto u : users) {
      if (u.login != "server" && u.login.substr(0, 6) != "client") {
        loginned = false;
        break;
      } else {
        loginned = true;
      }
    }
    if (loginned && users.size() > clients_count) {
      break;
    }
    logger("server.client.testForReconnection. not all clients was loggined");
  }

  for (auto &c : clients) {
    c->disconnect();
    while (c->is_connected()) {
      logger("server.client.testForReconnection. client is still connected");
    }
  }

  server_stop = true;
  while (server != nullptr) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  t.join();
}
} // namespace server_client_test

TEST_CASE("server.client.1") {
  const size_t connections_count = 1;
  server_client_test::testForReconnection(connections_count);
}

TEST_CASE("server.client.10") {
  const size_t connections_count = 10;
  server_client_test::testForReconnection(connections_count);
}

TEST_CASE("server.client.create_queue") {
  using namespace server_client_test;

  server_stop = false;
  std::thread t(server_thread);

  while (server == nullptr || !server->is_started()) {
    logger("server.client.create_queue !server->is_started serverIsNull? ",
           server == nullptr);
  }

  auto client = std::make_shared<Client>(
      service, AbstractClient::Params("client1", "localhost", 4040));
  auto client2 = std::make_shared<Client>(
      service, AbstractClient::Params("client2", "localhost", 4040));
  client->connectAsync();
  client2->connectAsync();
  while (!client->is_connected() || !client2->is_connected()) {
    logger("server.client.create_queue client not connected");
  }

  while (true) {
    auto us = server->users();
    if (us.size() == size_t(3)) {
      break;
    } else {
      logger("server.client.create_queue erver->getClientDescription is empty ",
             server == nullptr);
    }
  }

  auto qname = "server.client.create_queue1";
  QueueSettings qsettings1(qname);
  client->createQueue(qsettings1);

  while (true) {
    auto ds = server->getDescription();
    if (!ds.empty()) {
      EXPECT_EQ(ds.front().settings.name, qname);
      break;
    }
    logger("server.client.create_queue server->getDescription is empty");
  }

  auto test_data = std::vector<uint8_t>{0, 1, 2, 3, 4, 5, 6};
  int sended = 0;
  DataHandler handler = [&test_data, &sended, &qname](const MessageInfo&info,
                                                      const rawData &d, Id) {
    EXPECT_TRUE(std::equal(test_data.begin(), test_data.end(), d.begin(), d.end()));
    EXPECT_EQ(info.queueName, qname);
    sended++;
  };

  LambdaEventConsumer clientConsumer(handler);
  client->subscribe(qname, &clientConsumer);
  client2->subscribe(qname, &clientConsumer);

  while (true) {
    auto ds = server->getDescription();
    if (!ds.empty() && !ds.front().subscribers.empty()) {
      EXPECT_EQ(ds.front().settings.name, qname);
      break;
    }
    logger("server.client.create_queue server->getDescription is empty");
  }

  DataHandler server_handler = [&test_data, &sended, &qname](const MessageInfo&info,
                                                             const rawData &d, Id) {
    EXPECT_TRUE(std::equal(test_data.begin(), test_data.end(), d.begin(), d.end()));
    EXPECT_EQ(info.queueName, qname);
    sended++;
  };

  LambdaEventConsumer serverConsumer(server_handler);

  server->subscribe(qname, &serverConsumer);

  client->publish(qname, test_data);
  while (sended != int(3) && client->messagesInPool() != size_t(0)) {
    logger("server.client.create_queue !sended");
  }

  client->unsubscribe(qname);
  client2->disconnect();

  while (true) {
    auto ds = server->getDescription();
    if (ds.empty()) {
      break;
    }
    logger("server.client.create_queue server->getDescription is not empty");
  }

  server_stop = true;
  while (server != nullptr) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  t.join();
}

TEST_CASE("server.client.empty_queue-erase") {
  using namespace server_client_test;

  server_stop = false;
  std::thread t(server_thread);

  while (server == nullptr || !server->is_started()) {
    logger("server.client.empty_queue-erase !server->is_started serverIsNull? ",
           server == nullptr);
  }

  auto client = std::make_shared<Client>(
      service, AbstractClient::Params("client1", "localhost", 4040));
  auto client2 = std::make_shared<Client>(
      service, AbstractClient::Params("client2", "localhost", 4040));

  client->connect();
  client2->connect();
  EXPECT_TRUE(client->is_connected());
  EXPECT_TRUE(client2->is_connected());
  EXPECT_FALSE(client->getId() == client2->getId());

  auto qname = "server.client.empty_queue-erase";
  QueueSettings qsettings1(qname);
  client->createQueue(qsettings1);

  while (true) {
    auto ds = server->getDescription();
    if (!ds.empty()) {
      EXPECT_EQ(ds.front().settings.name, qname);
      break;
    }
    logger("server.client.empty_queue-erase server->getDescription is empty");
  }

  client->subscribe(qname, nullptr);
  client2->subscribe(qname, nullptr);

  while (true) {
    auto ds = server->getDescription();
    if (!ds.empty() && ds.front().subscribers.size() == size_t(2)) {
      EXPECT_EQ(ds.front().settings.name, qname);
      break;
    }
    logger("server.client.empty_queue-erase server->getDescription is empty");
  }

  client->disconnect();
  client2->disconnect();

  while (client->is_connected() || client2->is_connected()) {
    logger("server.client.empty_queue-erase client is still connected");
  }

  while (true) {
    auto ds = server->getDescription();
    if (ds.empty()) {
      break;
    }
    logger("server.client.empty_queue-erase server->getDescription is not empty");
  }
  client = nullptr;
  client2 = nullptr;
  server_stop = true;
  while (server != nullptr) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  t.join();
}

TEST_CASE("server.client.server-side-queue") {
  using namespace server_client_test;

  server_stop = false;
  std::thread t(server_thread);

  while (server == nullptr || !server->is_started()) {
    logger("server.client.empty_queue-erase !server->is_started serverIsNull? ",
           server == nullptr);
  }

  auto client = std::make_shared<Client>(
      service, AbstractClient::Params("client", "localhost", 4040));

  client->connect();

  EXPECT_TRUE(client->is_connected());

  auto qname = "server.client.server-side-queue";
  QueueSettings qsettings1(qname);
  server->createQueue(qsettings1);

  while (true) {
    auto ds = server->getDescription();
    if (!ds.empty()) {
      EXPECT_EQ(ds.front().settings.name, qname);
      break;
    }
    logger("server.client.empty_queue-erase server->getDescription is empty");
  }

  client->subscribe(qname, nullptr);

  while (true) {
    auto ds = server->getDescription();
    if (!ds.empty() && ds.front().subscribers.size() == size_t(2)) {
      EXPECT_EQ(ds.front().settings.name, qname);
      break;
    }
    logger("server.client.empty_queue-erase server->getDescription is empty");
  }

  client->disconnect();

  while (client->is_connected()) {
    logger("server.client.empty_queue-erase client is still connected");
  }

  auto ds = server->getDescription();
  EXPECT_EQ(ds.size(), size_t(1));

  server_stop = true;
  while (server != nullptr) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  t.join();
}

TEST_CASE("server.client.publish-from-pool") {
  using namespace server_client_test;

  server_stop = false;
  std::thread t(server_thread);

  while (server == nullptr || !server->is_started()) {
    logger("server.client.publish-from-pool !server->is_started serverIsNull? ",
           server == nullptr);
  }

  auto client = std::make_shared<Client>(
      service, AbstractClient::Params("client", "localhost", 4040, true));
  client->connect();
  EXPECT_TRUE(client->is_connected());

  auto qname = "server.client.publish-from-pool";
  QueueSettings qsettings1(qname);
  client->createQueue(qsettings1);

  std::set<Id> ids;
  int sended = 0;
  DataHandler handler = [&sended, &ids, &qname](const MessageInfo&info,
                                                const rawData &, Id id) {
    EXPECT_EQ(info.queueName, qname);
    sended++;
    ids.insert(id);
  };
  LambdaEventConsumer clientConsumer(handler);
  client->subscribe(qname, &clientConsumer);

  auto client2 = std::make_shared<Client>(
      service, AbstractClient::Params("client2", "localhost", 4040));
  EXPECT_FALSE(client2->is_connected());
  client2->publish(qname, {1, 2, 3});
  client2->connect();
  EXPECT_TRUE(client2->is_connected());

  while (sended != 1 && client2->messagesInPool() != 0 && ids.size() > 1) {
    logger("server.client.publish-from-pool sended!=1");
  }

  server_stop = true;
  while (server != nullptr) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  t.join();
}
