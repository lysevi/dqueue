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
void server_thread() {
  boost::asio::io_service service;
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
  boost::asio::io_service service;
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
    service.poll_one();
  }

  for (auto &c : clients) {
    while (!c->is_connected()) {
      logger_info("server.client.testForReconnection. client not connected");
      service.poll_one();
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
  boost::asio::io_service service;
  AbstractClient::Params p;
  p.host = "localhost";
  p.port = 4040;

  server_stop = false;
  std::thread t(server_thread);

  auto client = std::make_shared<Client>(&service, p);
  client->asyncConnect();
  while (!client->is_connected()) {
    logger_info("server.client.create_queue client not connected");
    service.poll_one();
  }

  while (server == nullptr || !server->is_started()) {
    logger_info("server.client.create_queue !server->is_started serverIsNull? ",
                server == nullptr);
  }

  while (true) {
    auto cds = server->getClientDescription();
    if (cds.size() == size_t(1)) {
      break;
    } else {
      service.poll_one();
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
    service.poll_one();
  }

  client->subscribe(qname);

  while (true) {
    auto ds = server->getDescription();
    if (!ds.empty() && !ds.front().subscribers.empty()) {
      EXPECT_EQ(ds.front().settings.name, qname);
      break;
    }
    logger_info("server.client.create_queue server->getDescription is empty");
    service.poll_one();
  }

  auto test_data = std::vector<uint8_t>{0, 1, 2, 3, 4, 5, 6};
  bool sended = false;
  Client::dataHandler handler = [&test_data, &sended](const Queue &q,
                                                      const std::vector<uint8_t> &data) {
    EXPECT_TRUE(std::equal(test_data.begin(), test_data.end(), data.begin(), data.end()));
    sended = true;
  };

  client->addHandler(handler);

  client->publish(qname, test_data);
  while (!sended) {
    logger_info("server.client.create_queue !sended");
    service.poll_one();
  }

  client->unsubscribe(qname);

  while (true) {
    auto ds = server->getDescription();
    if (ds.empty() || !ds.empty() && ds.front().subscribers.empty()) {
      break;
    }
    logger_info("server.client.create_queue server->getDescription is not empty");
    service.poll_one();
  }

  server_stop = true;
  while (server != nullptr) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  t.join();
}
