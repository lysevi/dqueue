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

namespace {

bool server_stop = false;
std::shared_ptr<Server> server = nullptr;
boost::asio::io_service service;
void server_thread() {
  AbstractServer::params p;
  p.port = 4040;
  server = std::make_shared<Server>(&service, p);

  server->serverStart();
  while (!server_stop) {
    service.poll_one();
  }
  server->stopServer();
  server = nullptr;
}

void testForReconnection(const size_t clients_count) {
  AbstractClient::Params p;
  p.host = "localhost";
  p.port = 4040;
  std::vector<std::shared_ptr<Client>> clients(clients_count);
  for (size_t i = 0; i < clients_count; i++) {
    clients[i] = std::make_shared<Client>(&service, p);
    clients[i]->asyncConnect();
  }
  server_stop = false;
  std::thread t(server_thread);
  while (server == nullptr || !server->is_started()) {
    logger_info("server.client.testForReconnection. !server->is_started serverIsNull? ",
                server == nullptr);
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
} // namespace

TEST_CASE("server.client.1") {
  const size_t connections_count = 1;
  testForReconnection(connections_count);
}

TEST_CASE("server.client.10") {
  const size_t connections_count = 10;
  testForReconnection(connections_count);
}

TEST_CASE("server.client.create_queue") {
  AbstractClient::Params p;
  p.host = "localhost";
  p.port = 4040;

  server_stop = false;
  std::thread t(server_thread);

  auto client = std::make_shared<Client>(&service, p);
  auto client2 = std::make_shared<Client>(&service, p);
  client->asyncConnect();
  client2->asyncConnect();
  while (!client->is_connected() || !client2->is_connected()) {
    logger_info("server.client.create_queue client not connected");
  }

  while (server == nullptr || !server->is_started()) {
    logger_info("server.client.create_queue !server->is_started serverIsNull? ",
                server == nullptr);
  }

  while (true) {
    auto cds = server->getClientDescription();
    if (cds.size() == size_t(2)) {
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
  Client::dataHandler handler = [&test_data, &sended](const Queue &q,
                                                      const std::vector<uint8_t> &data) {
    EXPECT_TRUE(std::equal(test_data.begin(), test_data.end(), data.begin(), data.end()));
    sended++;
  };

  client->addHandler(handler);
  client2->addHandler(handler);

  client->publish(qname, test_data);
  while (sended != int(2)) {
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
  AbstractClient::Params p;
  p.host = "localhost";
  p.port = 4040;

  server_stop = false;
  std::thread t(server_thread);

  auto client = std::make_shared<Client>(&service, p);
  auto client2 = std::make_shared<Client>(&service, p);
  client->asyncConnect();
  client2->asyncConnect();

  while (!client->is_connected() || !client2->is_connected()) {
    logger_info("server.client.empty_queue-erase client not connected");
  }

  while (server == nullptr || !server->is_started()) {
    logger_info("server.client.empty_queue-erase !server->is_started serverIsNull? ",
                server == nullptr);
  }

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
    if (!ds.empty() && !ds.front().subscribers.empty()) {
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
    if (ds.empty() ) {
      break;
    }
    logger_info("server.client.empty_queue-erase server->getDescription is not empty");
  }

  server_stop = true;
  while (server != nullptr) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  t.join();
}
