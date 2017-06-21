#include <libdqueue/node.h>
#include <libdqueue/queries.h>
#include <libdqueue/server.h>

#include <functional>
#include <string>

using namespace dqueue;

struct Server::Private final : public AbstractServer, public IQueueClient {
  Private(boost::asio::io_service *service, AbstractServer::params &p)
      : AbstractServer(service, p) {
    DataHandler dhandler = [this](const std::string &queueName, const rawData &rd,
                                  int id) { this->onSendToClient(queueName, rd, id); };
    _node = std::make_unique<Node>(Node::Settings(), dhandler);
    _node->addClient({Node::ServerID});
  }

  virtual ~Private() { _node->eraseClient(Node::ServerID); }

  void onMessageSended(ClientConnection &i, const NetworkMessage_ptr &d) {}

  void onNetworkError(ClientConnection &i, const NetworkMessage_ptr &d,
                      const boost::system::error_code &err) {
    bool operation_aborted = err == boost::asio::error::operation_aborted;
    bool eof = err == boost::asio::error::eof;
    if (!operation_aborted && !eof) {
      std::string errm = err.message();
      logger_fatal("server: ", errm);
    }
  }

  void onSendToClient(const std::string &queueName, const rawData &rd, int id) {
    if (id == Node::ServerID) {
      if (_dh != nullptr) {
        _dh(queueName, rd, id);
      }
    } else {
      // TODO make one allocation in onNewMessage
      queries::Publish pub(queueName, rd);
      auto nd = pub.toNetworkMessage();
      this->sendTo(id, nd);
    }
  }

  void onNewMessage(ClientConnection &i, const NetworkMessage_ptr &d, bool &cancel) {
    auto hdr = d->cast_to_header();

    switch (hdr->kind) {
    case (NetworkMessage::message_kind)MessageKinds::CREATE_QUEUE: {
      logger_info("server: #", i.get_id(), " create queue");
      queries::CreateQueue cq(d);
      QueueSettings qs(cq.name);
      _node->createQueue(qs, i.get_id());
      break;
    }
    case (NetworkMessage::message_kind)MessageKinds::SUBSCRIBE: {
      logger_info("server: #", i.get_id(), " subscribe");
      auto cs = queries::ChangeSubscribe(d);
      _node->changeSubscription(Node::SubscribeActions::Subscribe, cs.qname, i.get_id());
      break;
    }
    case (NetworkMessage::message_kind)MessageKinds::UNSUBSCRIBE: {
      logger_info("server: #", i.get_id(), " unsubscribe");
      auto cs = queries::ChangeSubscribe(d);
      _node->changeSubscription(Node::SubscribeActions::Unsubscribe, cs.qname,
                                i.get_id());
      break;
    }
    case (NetworkMessage::message_kind)MessageKinds::PUBLISH: {
      logger("server: #", i.get_id(), " publish");
      auto cs = queries::Publish(d);
      _node->publish(cs.qname, cs.data);
      break;
    }
    default:
      THROW_EXCEPTION("unknow message kind: ", hdr->kind);
    }
  }

  ON_NEW_CONNECTION_RESULT onNewConnection(ClientConnection &i) {
    // TODO logic must be implemented in call code
    Node::Client cl;
    cl.id = i.get_id();
    _node->addClient(cl);
    return ON_NEW_CONNECTION_RESULT::ACCEPT;
  }

  void onDisconnect(const ClientConnection &i) override {
    _node->eraseClient(i.get_id());
  }

  std::vector<Node::QueueDescription> getDescription() const {
    return _node->getQueuesDescription();
  }

  std::vector<Node::ClientDescription> getClientDescription() const {
    return _node->getClientsDescription();
  }

  void createQueue(const QueueSettings &settings) override {
    _node->createQueue(settings, Node::ServerID);
  }

  void addHandler(DataHandler dh) { _dh = dh; }

  void subscribe(const std::string &qname) override {
    _node->changeSubscription(Node::SubscribeActions::Subscribe, qname, Node::ServerID);
  }

  void unsubscribe(const std::string &qname) override {
    _node->changeSubscription(Node::SubscribeActions::Unsubscribe, qname, Node::ServerID);
  }

  virtual void publish(const std::string &qname, const rawData &data) override {
    _node->publish(qname, data);
  }

  std::unique_ptr<Node> _node;
  DataHandler _dh;
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

std::vector<Node::ClientDescription> Server::getClientDescription() const {
  return _impl->getClientDescription();
}

void Server::addHandler(DataHandler dh) {
  return _impl->addHandler(dh);
}

void Server::createQueue(const QueueSettings &settings) {
  return _impl->createQueue(settings);
}

void Server::subscribe(const std::string &qname) {
  return _impl->subscribe(qname);
}

void Server::unsubscribe(const std::string &qname) {
  return _impl->unsubscribe(qname);
}

void Server::publish(const std::string &qname, const rawData &data) {
  return _impl->publish(qname, data);
}
