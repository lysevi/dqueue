#include <libdqueue/server.h>

#include <functional>
#include <string>

using namespace dqueue;

struct Server::Private final : public AbstractServer {
  Private(boost::asio::io_service *service, AbstractServer::params &p) :AbstractServer(service, p) {
  }

virtual ~Private() {}

  void onMessageSended(ClientConnection &i, const NetworkMessage_ptr &d) {}

  void onNetworkError(ClientConnection &i, const NetworkMessage_ptr &d,
                      const boost::system::error_code &err) {}

  void onNewMessage(ClientConnection &i, const NetworkMessage_ptr &d, bool &cancel) {}

  ON_NEW_CONNECTION_RESULT onNewConnection(ClientConnection &i) {
	  //TODO logic must be implemented in call code
	  return ON_NEW_CONNECTION_RESULT::ACCEPT;
  }
};

Server::Server(boost::asio::io_service *service, AbstractServer::params &p)
    : _impl(std::make_shared<Private>(service, p)) {}

Server::~Server() {
  _impl = nullptr;
}

void Server::serverStart() {
  _impl->serverStart();
}

void Server::stopServer() {
	_impl->stopServer();
}

bool Server::is_started() {
	return _impl->is_started();
}