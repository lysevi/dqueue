#include "helpers.h"
#include <libdqueue/async_connection.h>
#include <libdqueue/abstract_client.h>
#include <libdqueue/abstract_server.h>
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


struct testable_client:public abstract_client {
	size_t message_one = 0;

	testable_client(boost::asio::io_service *service) : abstract_client(service) {
	}

	void onNewMessage(const network_message_ptr &d, bool & /*cancel*/) override{
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

struct testable_server :public abstract_server {
  size_t message_one = 0;
 
  testable_server(boost::asio::io_service *service, const abstract_server::params&p) : abstract_server(service,p) {
  }

 /* void onNetworkError(const boost::system::error_code &err) {
    THROW_EXCEPTION("error on - ", err.message());
  }*/

  
  void onNewMessage(abstract_server::io&io, const network_message_ptr &d, bool &cancel) {
    auto qh = reinterpret_cast<message_header *>(d->data);

    int kind = (network_message::message_kind)qh->kind;
    switch (kind) {
    case 1: {
      message_one++;
	  io._async_connection->send(d);
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
  abstract_server::params p;
  p.port = 4040;
  server = new testable_server(&service,p);

  server->serverStart();
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
  while (server == nullptr || !server->is_started) {
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