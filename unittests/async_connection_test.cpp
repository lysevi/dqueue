#include "helpers.h"
#include <libdqueue/abstract_client.h>
#include <libdqueue/abstract_server.h>
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
namespace {
	struct testable_client : public AbstractClient {
		size_t message_one = 0;

		testable_client(boost::asio::io_service *service, const AbstractClient::Params &p)
			: AbstractClient(service, p) {}

		~testable_client() {}

		void onConnect() {
			logger_info("client: send hello ");

			auto nd = std::make_shared<NetworkMessage>(1);

			this->_async_connection->send(nd);
		}

		void onNewMessage(const NetworkMessage_ptr &d, bool & /*cancel*/) override {
			auto qh = reinterpret_cast<message_header *>(d->data);

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
		size_t message_one = 0;

		testable_server(boost::asio::io_service *service, const AbstractServer::params &p)
			: AbstractServer(service, p) {}

		~testable_server() {}
		/* void onNetworkError(const boost::system::error_code &err) {
		   THROW_EXCEPTION("error on - ", err.message());
		 }*/

		void onNewMessage(AbstractServer::io &io, const NetworkMessage_ptr &d, bool &cancel) {
			auto qh = reinterpret_cast<message_header *>(d->data);

			int kind = (NetworkMessage::message_kind)qh->kind;
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

		void onNetworkError(const boost::system::error_code &err) override {}
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
		while (server == nullptr || !server->is_started) {
			logger_info("!server->is_started serverIsNull? ", server == nullptr);
			service.poll_one();
		}

		for (auto&c : clients) {
			while (!c->isConnected) {
				logger_info("client not connected");
				service.poll_one();
			}
		}
		for (auto&c : clients) {
			while (server->message_one < 10 && c->message_one < 10) {
				logger_info("server.message_one: ", server->message_one,
					" client.message_one: ", c->message_one);
				service.poll_one();
			}
		}
		server_stop = true;
		while (server != nullptr) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		t.join();

		for (auto&c : clients) {
			while (c->isConnected) {
				logger_info("client is still connected");
				service.poll_one();
			}
		}

		// wait auto recconection on client.
		server_stop = false;
		t = std::thread(server_thread);
		for (auto&c : clients) {
			while (!c->isConnected && (server == nullptr || !server->is_started)) {
				logger_info("client and server is not connected");
				service.poll_one();
			}
		}

		// disconnect from server
		for (auto&c : clients) {
			c->disconnect();
		}
		for (auto&c : clients) {
			while (c->isConnected) {
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