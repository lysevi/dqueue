#include "helpers.h"
#include <libdqueue/abstract_client.h>
#include <libdqueue/abstract_server.h>
#include <libdqueue/async_io.h>
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
struct testable_client : public AbstractClient {
  size_t message_one = 0;

  testable_client(io_service *service, const AbstractClient::Params &p)
      : AbstractClient(service, p) {}

  ~testable_client() {}

  void onConnect() override {
    logger_info("client: send hello ");

    auto nd = std::make_shared<NetworkMessage>(1);

    this->_async_connection->send(nd);
  }

  bool success = false;
  void onMessageSended(const NetworkMessage_ptr &) override { success = true; }

  virtual void onNetworkError(const NetworkMessage_ptr &,
                              const boost::system::error_code &) override {}

  void onNewMessage(const NetworkMessage_ptr &d, bool & /*cancel*/) override {
	auto qh = d->cast_to_header();

    int kind = (NetworkMessage::message_kind)qh->kind;
    switch (kind) {
    case 1: {
      message_one++;
      this->_async_connection->send(d);
      break;
    }
    default:
      dqueue::logger_fatal("server: unknow query kind - ", (int)kind);
      break;
    }
  }
};

struct testable_server : public AbstractServer {
  std::map<int, size_t> id2count;
  std::mutex _locker;

  bool all_id_gt(size_t v) {
    std::lock_guard<std::mutex> lg(_locker);
    if (id2count.empty()) {
      return false;
    }
    bool result = true;
    for (const auto &kv : id2count) {
      if (kv.second < v) {
        result = false;
        break;
      }
    }
    return result;
  }

  testable_server(io_service *service, const AbstractServer::params &p)
      : AbstractServer(service, p) {}

  virtual ~testable_server() { logger("stop testable server"); }
  bool success = false;
  void onMessageSended(AbstractServer::ClientConnection &,
                       const NetworkMessage_ptr &) override {
    success = true;
  }

  void onNewMessage(AbstractServer::ClientConnection &ClientConnection,
                    const NetworkMessage_ptr &d, bool &) {
    auto qh = d->cast_to_header();

    int kind = (NetworkMessage::message_kind)qh->kind;
    switch (kind) {
    case 1: {
      std::lock_guard<std::mutex> lg(_locker);
      auto fres = id2count.find(ClientConnection.get_id());
      if (fres == id2count.end()) {
        id2count[ClientConnection.get_id()] = size_t();
      } else {
        fres->second += 1;
      }
      ClientConnection.sendData(d);
      break;
    }
    default:
      dqueue::logger_fatal("server: unknow query kind - ", (int)kind);
      break;
    }
  }

  void onNetworkError(ClientConnection &, const NetworkMessage_ptr &,
                      const boost::system::error_code &) override {}

  ON_NEW_CONNECTION_RESULT onNewConnection(ClientConnection &)override {
	  return ON_NEW_CONNECTION_RESULT::ACCEPT;
  }
};

bool server_stop = false;
std::shared_ptr<testable_server> server = nullptr;
void server_thread() {
  boost::asio::io_service service;
  AbstractServer::params p;
  p.port = 4040;
  server = std::make_shared<testable_server>(&service, p);

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
  std::vector<std::shared_ptr<testable_client>> clients(clients_count);
  for (size_t i = 0; i < clients_count; i++) {
    clients[i] = std::make_shared<testable_client>(&service, p);
    clients[i]->async_connect();
  }
  server_stop = false;
  std::thread t(server_thread);
  while (server == nullptr || !server->is_started()) {
    logger_info("!server->is_started serverIsNull? ", server == nullptr);
    service.poll_one();
  }

  for (auto &c : clients) {
    while (!c->is_connected()) {
      logger_info("client not connected");
      service.poll_one();
    }
  }
  for (auto &c : clients) {
    while (!server->all_id_gt(10) && c->message_one < 10) {
      logger_info("client.message_one: ", c->message_one);
      service.poll_one();
    }
  }
  EXPECT_TRUE(server->success);
  server_stop = true;
  while (server != nullptr) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  t.join();

  for (auto &c : clients) {
    EXPECT_TRUE(c->success);
    while (c->is_connected()) {
      logger_info("client is still connected");
      service.poll_one();
    }
  }

  // wait auto recconection on client.
  server_stop = false;
  t = std::thread(server_thread);
  for (auto &c : clients) {
    while (!c->is_connected() && (server == nullptr || !server->is_started())) {
      logger_info("client and server is not connected");
      service.poll_one();
    }
  }

  // disconnect from server
  for (auto &c : clients) {
    c->disconnect();
  }
  for (auto &c : clients) {
    while (c->is_connected()) {
      logger_info("client is still connected");
      service.poll_one();
    }
  }
  // stop server
  server_stop = true;
  while (server != nullptr) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  t.join();
}
}

TEST_CASE("reconnetion.1") {
  const size_t connections_count = 1;
  testForReconnection(connections_count);
}

TEST_CASE("reconnetion.10") {
  const size_t connections_count = 10;
  testForReconnection(connections_count);
}
