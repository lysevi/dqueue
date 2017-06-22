#include <libdqueue/node.h>
#include <libdqueue/queries.h>
#include <libdqueue/server.h>
#include <libdqueue/users.h>

#include <functional>
#include <string>

using namespace dqueue;

struct Server::Private final : public AbstractServer, public IQueueClient {
  Private(boost::asio::io_service *service, AbstractServer::params &p)
      : AbstractServer(service, p) {
    DataHandler dhandler = [this](const std::string &queueName, const rawData &rd,
                                  Id id) { this->onSendToClient(queueName, rd, id); };
    _users = UserBase::create();
    _node = std::make_unique<Node>(Node::Settings(), dhandler, _users);
    _users->append({"server", ServerID});
  }

  virtual ~Private() { _node->eraseClient(ServerID); }

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

  void onSendToClient(const std::string &queueName, const rawData &rd, Id id) {
    if (id == ServerID) {
      if (_dh != nullptr) {
        _dh(queueName, rd, id);
      }
    } else {
      std::lock_guard<std::mutex> lg(_locker);
      // TODO make one allocation in onNewMessage
      queries::Publish pub(queueName, rd, _nextMessageId++);
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
      sendOk(i, cs.messageId);
      break;
    }
    case (NetworkMessage::message_kind)MessageKinds::LOGIN: {
      logger_info("server: #", i.get_id(), " set login");
      queries::Login lg(d);
      _users->setLogin(i.get_id(), lg.login);
      sendOk(i, LoginConfirmedID);
      break;
    }
    default:
      THROW_EXCEPTION("unknow message kind: ", hdr->kind);
    }
  }

  void sendOk(ClientConnection &i, uint64_t messageId) {
    auto nd = queries::Ok(messageId).toNetworkMessage();
    this->sendTo(i, nd);
  }

  ON_NEW_CONNECTION_RESULT onNewConnection(ClientConnection &i) {
    // TODO logic must be implemented in call code
    User cl;
    cl.id = i.get_id();
    cl.login = "not set";
    _users->append(cl);
    return ON_NEW_CONNECTION_RESULT::ACCEPT;
  }

  void onDisconnect(const ClientConnection &i) override {
    _users->erase(i.get_id());
    _node->eraseClient(i.get_id());
  }

  std::vector<Node::QueueDescription> getDescription() const {
    return _node->getQueuesDescription();
  }

  std::vector<User> users() const { return _users->users(); }

  void createQueue(const QueueSettings &settings) override {
    _node->createQueue(settings, ServerID);
  }

  void addHandler(DataHandler dh) { _dh = dh; }

  void subscribe(const std::string &qname) override {
    _node->changeSubscription(Node::SubscribeActions::Subscribe, qname, ServerID);
  }

  void unsubscribe(const std::string &qname) override {
    _node->changeSubscription(Node::SubscribeActions::Unsubscribe, qname, ServerID);
  }

  virtual void publish(const std::string &qname, const rawData &data) override {
    _node->publish(qname, data);
  }

  std::mutex _locker;
  uint64_t _nextMessageId = 0;
  std::unique_ptr<Node> _node;
  DataHandler _dh;
  UserBase_Ptr _users;
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

std::vector<User> Server::users() const {
  return _impl->users();
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
