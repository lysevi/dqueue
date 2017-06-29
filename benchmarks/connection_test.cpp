#include <boost/asio.hpp>
#include <exception>
#include <functional>
#include <iostream>
#include <thread>
#include <list>

using namespace std::placeholders;
using namespace boost::asio;
using namespace boost::asio::ip;

std::unique_ptr<boost::asio::io_service> server_service;
std::unique_ptr<boost::asio::io_service> client_service;

typedef std::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;
bool stop_threads = false;
void server_thread() {
  while (!stop_threads) {
    server_service->run_one();
  }
}

void client_thread() {
  while (!stop_threads) {
    client_service->run_one();
  }
}

void handle_accept(socket_ptr sock, const boost::system::error_code &err) {
  if (err) {
    if (err == boost::asio::error::operation_aborted ||
        err == boost::asio::error::connection_reset || err == boost::asio::error::eof) {
      return;
    } else {
      throw std::logic_error("dqueue::server: error on accept " + err.message());
    }
  } else {
    std::cout << "server: accept connection.\n";
  }
}

int main(int argc, char *argv[]) {
  server_service = std::make_unique<boost::asio::io_service>();
  client_service = std::make_unique<boost::asio::io_service>();
  std::list<std::thread> threads;

  threads.emplace_back(&server_thread);
  threads.emplace_back(&client_thread);

  auto new_socket = std::make_shared<boost::asio::ip::tcp::socket>(*server_service);
  tcp::endpoint ep(tcp::v4(), 4040);
  boost::asio::ip::tcp::acceptor acc(*server_service, ep);
  acc.async_accept(*new_socket, std::bind(&handle_accept, new_socket, _1));

  tcp::resolver resolver(*client_service);
  tcp::resolver::query query("localhost", "4040", tcp::resolver::query::canonical_name);
  tcp::resolver::iterator iter = resolver.resolve(query);

  for (; iter != tcp::resolver::iterator(); ++iter) {
    auto localep = iter->endpoint();
    if (localep.protocol() == tcp::v4()) {
      break;
    }
  }
  if (iter == tcp::resolver::iterator()) {
    throw std::logic_error("hostname not found.");
  }
  tcp::endpoint client_ep = *iter;
  bool is_connected = false;
  auto client_socket = std::make_shared<boost::asio::ip::tcp::socket>(*client_service);
  client_socket->async_connect(client_ep, [&is_connected](auto ec) {
    if (ec) {
      std::cout << "client connection error: " << ec.message() << std::endl;
    } else {
      std::cout << "client: connected.";
      is_connected = true;
    }
  });

  while (!is_connected) {
    std::cout << "! is_connected" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  stop_threads = true;
  for (auto &it : threads) {
    it.join();
  }
}
