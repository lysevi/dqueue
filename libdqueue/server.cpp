#include <libdqueue/kinds.h>
#include <libdqueue/node.h>
#include <libdqueue/server.h>

#include <functional>
#include <string>

using namespace dqueue;

struct Server::Private final : public AbstractServer {
  Private(boost::asio::io_service *service, AbstractServer::params &p)
      : AbstractServer(service, p) {
    _node = std::make_unique<Node>(Node::Settings());
  }

  virtual ~Private() {}

  void onMessageSended(ClientConnection &i, const NetworkMessage_ptr &d) {}

  void onNetworkError(ClientConnection &i, const NetworkMessage_ptr &d,
                      const boost::system::error_code &err) {}

  void onNewMessage(ClientConnection &i, const NetworkMessage_ptr &d, bool &cancel) {
    auto hdr = d->cast_to_header();

    switch (hdr->kind) {
    case (NetworkMessage::message_kind)MessageKinds::CREATE_QUEUE: {
      auto qs = QueueSettings::fromNetworkMessage(d);
      _node->createQueue(qs);
      break;
    }
    default:
      THROW_EXCEPTION("unknow message kind: ", hdr->kind);
    }
  }

  ON_NEW_CONNECTION_RESULT onNewConnection(ClientConnection &i) {
    // TODO logic must be implemented in call code
    return ON_NEW_CONNECTION_RESULT::ACCEPT;
  }

  std::vector<Node::QueueDescription> getDescription() const {
    return _node->getDescription();
  }

  std::unique_ptr<Node> _node;
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

std::vector<Node::QueueDescription> Server::getDescription() const {
  return _impl->getDescription();
}