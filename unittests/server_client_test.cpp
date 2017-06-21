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
  AbstractClient::Params p;
  p.host = "localhost";
  p.port = 4040;

  server_stop = false;
  std::thread t(server_thread);
  while (server == nullptr || !server->is_started()) {
    logger_info("server.client.testForReconnection. !server->is_started serverIsNull? ",
                server == nullptr);
  }

  std::vector<std::shared_ptr<Client>> clients(clients_count);
  for (size_t i = 0; i < clients_count; i++) {
    clients[i] = std::make_shared<Client>(service, p);
    clients[i]->asyncConnect();
  }

  for (auto &c : clients) {
    while (!c->is_connected()) {
      logger_info("server.client.testForReconnection. client not connected");
    }
  }

  for (auto &c : clients) {
    c->disconnect();
    while (c->is_connected()) {
      logger_info("server.client.testForReconnection. client is still connected");
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

  AbstractClient::Params p;
  p.host = "localhost";
  p.port = 4040;

  server_stop = false;
  std::thread t(server_thread);

  while (server == nullptr || !server->is_started()) {
    logger_info("server.client.create_queue !server->is_started serverIsNull? ",
                server == nullptr);
  }

  auto client = std::make_shared<Client>(service, p);
  auto client2 = std::make_shared<Client>(service, p);
  client->asyncConnect();
  client2->asyncConnect();
  while (!client->is_connected() || !client2->is_connected()) {
    logger_info("server.client.create_queue client not connected");
  }

  while (true) {
    auto us = server->users();
    if (us.size() == size_t(3)) {
      break;
    } else {
      logger_info("server.client.create_queue erver->getClientDescription is empty ",
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
    logger_info("server.client.create_queue server->getDescription is empty");
  }

  client->subscribe(qname);
  client2->subscribe(qname);

  while (true) {
    auto ds = server->getDescription();
    if (!ds.empty() && !ds.front().subscribers.empty()) {
      EXPECT_EQ(ds.front().settings.name, qname);
      break;
    }
    logger_info("server.client.create_queue server->getDescription is empty");
  }

  auto test_data = std::vector<uint8_t>{0, 1, 2, 3, 4, 5, 6};
  int sended = 0;
  DataHandler handler = [&test_data, &sended, &qname](const std::string &queueName,
                                                      const rawData &d, Id id) {
    EXPECT_TRUE(std::equal(test_data.begin(), test_data.end(), d.begin(), d.end()));
    EXPECT_EQ(queueName, qname);
    sended++;
  };

  DataHandler server_handler = [&test_data, &sended, &qname](const std::string &queueName,
                                                             const rawData &d, Id id) {
    EXPECT_TRUE(std::equal(test_data.begin(), test_data.end(), d.begin(), d.end()));
    EXPECT_EQ(queueName, qname);
    sended++;
  };

  server->addHandler(server_handler);
  server->subscribe(qname);

  client->addHandler(handler);
  client2->addHandler(handler);

  client->publish(qname, test_data);
  while (sended != int(3)) {
    logger_info("server.client.create_queue !sended");
  }

  client->unsubscribe(qname);
  client2->disconnect();

  while (true) {
    auto ds = server->getDescription();
    if (ds.empty()) {
      break;
    }
    logger_info("server.client.create_queue server->getDescription is not empty");
  }

  server_stop = true;
  while (server != nullptr) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  t.join();
}

TEST_CASE("server.client.empty_queue-erase") {
  using namespace server_client_test;
  AbstractClient::Params p;
  p.host = "localhost";
  p.port = 4040;

  server_stop = false;
  std::thread t(server_thread);

  while (server == nullptr || !server->is_started()) {
    logger_info("server.client.empty_queue-erase !server->is_started serverIsNull? ",
                server == nullptr);
  }

  auto client = std::make_shared<Client>(service, p);
  auto client2 = std::make_shared<Client>(service, p);

  client->connect();
  client2->connect();
  EXPECT_TRUE(client->is_connected());
  EXPECT_TRUE(client2->is_connected());

  auto qname = "server.client.empty_queue-erase";
  QueueSettings qsettings1(qname);
  client->createQueue(qsettings1);

  while (true) {
    auto ds = server->getDescription();
    if (!ds.empty()) {
      EXPECT_EQ(ds.front().settings.name, qname);
      break;
    }
    logger_info("server.client.empty_queue-erase server->getDescription is empty");
  }

  client->subscribe(qname);
  client2->subscribe(qname);

  while (true) {
    auto ds = server->getDescription();
    if (!ds.empty() && ds.front().subscribers.size() == size_t(2)) {
      EXPECT_EQ(ds.front().settings.name, qname);
      break;
    }
    logger_info("server.client.empty_queue-erase server->getDescription is empty");
  }

  client->disconnect();
  client2->disconnect();

  while (client->is_connected() || client2->is_connected()) {
    logger_info("server.client.empty_queue-erase client is still connected");
  }

  while (true) {
    auto ds = server->getDescription();
    if (ds.empty()) {
      break;
    }
    logger_info("server.client.empty_queue-erase server->getDescription is not empty");
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
  AbstractClient::Params p;
  p.host = "localhost";
  p.port = 4040;

  server_stop = false;
  std::thread t(server_thread);

  while (server == nullptr || !server->is_started()) {
    logger_info("server.client.empty_queue-erase !server->is_started serverIsNull? ",
                server == nullptr);
  }

  auto client = std::make_shared<Client>(service, p);

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
    logger_info("server.client.empty_queue-erase server->getDescription is empty");
  }

  client->subscribe(qname);

  while (true) {
    auto ds = server->getDescription();
    if (!ds.empty() && ds.front().subscribers.size() == size_t(2)) {
      EXPECT_EQ(ds.front().settings.name, qname);
      break;
    }
    logger_info("server.client.empty_queue-erase server->getDescription is empty");
  }

  client->disconnect();

  while (client->is_connected()) {
    logger_info("server.client.empty_queue-erase client is still connected");
  }

  auto ds = server->getDescription();
  EXPECT_EQ(ds.size(), size_t(1));

  server_stop = true;
  while (server != nullptr) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  t.join();
}
