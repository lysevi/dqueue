#include "helpers.h"
#include <libdqueue/async_connection.h>
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


struct testable_client {
	size_t message_one = 0;
	std::shared_ptr<async_connection> _async_connection = nullptr;
	boost::asio::io_service *_service = nullptr;
	socket_ptr _socket = nullptr;
	std::shared_ptr<ip::tcp::acceptor> _acc = nullptr;
	bool call_accept = false;

	testable_client(boost::asio::io_service *service) : _service(service) {
		async_connection::onDataRecvHandler on_d = [this](const network_message_ptr &d, bool &cancel) {
			onDataRecv(d, cancel);
		};
		async_connection::onNetworkErrorHandler on_n =
			[this](const boost::system::error_code &err) { onNetworkError(err); };
		_async_connection = std::make_shared<async_connection>(on_d, on_n);
	}

	void onNetworkError(const boost::system::error_code &err) {
		THROW_EXCEPTION("error on - ", err.message());
	}

	void connectTo(const std::string &host, const std::string &port) {
		ip::tcp::resolver resolver(*_service);

		ip::tcp::resolver::query query(host, port, ip::tcp::resolver::query::canonical_name);
		ip::tcp::resolver::iterator iter = resolver.resolve(query);

		for (; iter != ip::tcp::resolver::iterator(); ++iter) {
			auto ep = iter->endpoint();
			if (ep.protocol() == ip::tcp::v4()) {
				break;
			}
		}
		if (iter == ip::tcp::resolver::iterator()) {
			THROW_EXCEPTION("hostname not found.");
		}
		ip::tcp::endpoint ep = *iter;
		logger_info("client: ", host, ":", port, " - ", ep.address().to_string());

		_socket = std::make_shared<ip::tcp::socket>(*_service);

		_socket->async_connect(ep, [this](auto ec) {
			if (ec) {
				auto msg = ec.message();
				THROW_EXCEPTION("dqueue::client: error on connect - ", msg);
			}
			this->_async_connection->start(this->_socket);

			logger_info("client: send hello ");

			auto nd = std::make_shared<network_message>(1);

			this->_async_connection->send(nd);
		});
	}

	void onDataRecv(const network_message_ptr &d, bool & /*cancel*/) {
		auto qh = reinterpret_cast<message_header *>(d->data);

		int kind = (network_message::message_kind)qh->kind;
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

struct testable_server {
  size_t message_one = 0;
  std::shared_ptr<async_connection> _async_connection = nullptr;
  boost::asio::io_service *_service = nullptr;
  socket_ptr _socket = nullptr;
  std::shared_ptr<ip::tcp::acceptor> _acc = nullptr;
  bool call_accept = false;

  testable_server(boost::asio::io_service *service) : _service(service) {
    async_connection::onDataRecvHandler on_d = [this](const network_message_ptr &d, bool &cancel) {
      onDataRecv(d, cancel);
    };
    async_connection::onNetworkErrorHandler on_n =
        [this](const boost::system::error_code &err) { onNetworkError(err); };
    _async_connection = std::make_shared<async_connection>(on_d, on_n);
  }

  void onNetworkError(const boost::system::error_code &err) {
    THROW_EXCEPTION("error on - ", err.message());
  }

  void serverStart(const unsigned short port) {
    ip::tcp::endpoint ep(ip::tcp::v4(), port);
    auto new_socket = std::make_shared<ip::tcp::socket>(*_service);
    _acc = std::make_shared<ip::tcp::acceptor>(*_service, ep);
    start_accept(new_socket);
    call_accept = true;
  }

  void start_accept(socket_ptr sock) {
    _acc->async_accept(*sock, std::bind(&testable_server::handle_accept, this, sock, _1));
  }

  void handle_accept(socket_ptr sock, const boost::system::error_code &err) {
    if (err) {
      THROW_EXCEPTION("dariadb::server: error on accept - ", err.message());
    }
    logger_info("server: accept connection.");
	_socket = sock;
    this->_async_connection->start(_socket);

    socket_ptr new_sock = std::make_shared<ip::tcp::socket>(*_service);
    start_accept(new_sock);
  }

  void onDataRecv(const network_message_ptr &d, bool & /*cancel*/) {
    auto qh = reinterpret_cast<message_header *>(d->data);

    int kind = (network_message::message_kind)qh->kind;
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

bool server_stop = false;
testable_server *server = nullptr;
void server_thread() {
  boost::asio::io_service service;
  server = new testable_server(&service);

  server->serverStart(4040);
  while (!server_stop) {
    service.poll_one();
  }
  delete server;
  server = nullptr;
}


TEST_CASE("async_connection") {
  boost::asio::io_service service;
  testable_client client(&service);
  std::thread t(server_thread);
  while (server == nullptr || !server->call_accept) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  client.connectTo("localhost", "4040");
  //std::thread t_client(client_thread, &service);

  while (server->message_one < 10 && client.message_one < 10) {
    logger_info("server.message_one: ", server->message_one,
                " client.message_one: ", client.message_one);
    service.poll_one();
  }
  server_stop = true;
  while (server != nullptr) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  t.join();  
}