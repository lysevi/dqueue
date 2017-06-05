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
    logger_info("!server->is_started serverIsNull? ", server == nullptr);
    service.poll_one();
  }

  for (auto &c : clients) {
    while (!c->is_connected()) {
      logger_info("client not connected");
      service.poll_one();
    }
  }
 
  server_stop = true;
  while (server != nullptr) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  t.join();
}
}

TEST_CASE("server.client.1") {
  const size_t connections_count = 1;
  testForReconnection(connections_count);
}

TEST_CASE("server.client.10") {
  const size_t connections_count = 10;
  testForReconnection(connections_count);
}
