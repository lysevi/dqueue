#include <libdqueue/node.h>
#include <libdqueue/queries.h>
#include <libdqueue/server.h>
#include <libdqueue/users.h>

#include <functional>
#include <string>

using namespace dqueue;

Server::Server(boost::asio::io_service *service, AbstractServer::params &p)
    : AbstractServer(service, p) {
  DataHandler dhandler = [this](const MessageInfo &info, const rawData &rd, Id id) {
    this->onSendToClient(info, rd, id);
  };
  _users = UserBase::create();
  _node = std::make_unique<Node>(Node::Settings(), dhandler, _users);
  _users->append({"server", ServerID});
}

Server::~Server() {
  _node->eraseClient(ServerID);
}

void Server::onMessageSended(ClientConnection_Ptr i, const NetworkMessage_ptr &d) {}

void Server::onNetworkError(ClientConnection_Ptr i, const NetworkMessage_ptr &d,
                            const boost::system::error_code &err) {
  bool operation_aborted = err == boost::asio::error::operation_aborted;
  bool eof = err == boost::asio::error::eof;
  if (!operation_aborted && !eof) {
    std::string errm = err.message();
    logger_fatal("server: ", errm);
  }
}

void Server::onSendToClient(const MessageInfo &info, const rawData &rd, Id id) {
  if (id == ServerID) {
    this->callConsumer(info, rd, id);
  } else {
    std::lock_guard<std::mutex> lg(_locker);
    // TODO make one allocation in onNewMessage
    queries::Publish pub(info.queueName, rd, _nextMessageId++);
    auto nd = pub.toNetworkMessage();
    this->sendTo(id, nd);
  }
}

void Server::onNewMessage(ClientConnection_Ptr i, const NetworkMessage_ptr &d,
                          bool &cancel) {
  auto hdr = d->cast_to_header();

  switch (hdr->kind) {
  case (NetworkMessage::message_kind)MessageKinds::CREATE_QUEUE: {
    logger_info("server: #", i->get_id(), " create queue");
    queries::CreateQueue cq(d);
    QueueSettings qs(cq.name);
    _node->createQueue(qs, i->get_id());
    break;
  }
  case (NetworkMessage::message_kind)MessageKinds::SUBSCRIBE: {
    logger_info("server: #", i->get_id(), " subscribe");
    auto cs = queries::ChangeSubscribe(d);
    _node->changeSubscription(Node::SubscribeActions::Subscribe, cs.qname, i->get_id());
    break;
  }
  case (NetworkMessage::message_kind)MessageKinds::UNSUBSCRIBE: {
    logger_info("server: #", i->get_id(), " unsubscribe");
    auto cs = queries::ChangeSubscribe(d);
    _node->changeSubscription(Node::SubscribeActions::Unsubscribe, cs.qname, i->get_id());
    break;
  }
  case (NetworkMessage::message_kind)MessageKinds::PUBLISH: {
    logger("server: #", i->get_id(), " publish");
    auto cs = queries::Publish(d);
    _node->publish(cs.qname, cs.data, i->get_id());
    sendOk(i, cs.messageId);
    break;
  }
  case (NetworkMessage::message_kind)MessageKinds::LOGIN: {
    logger_info("server: #", i->get_id(), " set login");
    queries::Login lg(d);
    _users->setLogin(i->get_id(), lg.login);

    queries::LoginConfirm lc(i->get_id());
    auto nd = lc.toNetworkMessage();
    sendTo(i, nd);
    break;
  }
  default:
    THROW_EXCEPTION("unknow message kind: ", hdr->kind);
  }
}

void Server::sendOk(ClientConnection_Ptr i, uint64_t messageId) {
  auto nd = queries::Ok(messageId).toNetworkMessage();
  this->sendTo(i, nd);
}

ON_NEW_CONNECTION_RESULT Server::onNewConnection(ClientConnection_Ptr i) {
  // TODO logic must be implemented in call code
  User cl;
  cl.id = i->get_id();
  cl.login = "not set";
  _users->append(cl);
  return ON_NEW_CONNECTION_RESULT::ACCEPT;
}

void Server::onDisconnect(const AbstractServer::ClientConnection_Ptr &i) {
  _users->erase(i->get_id());
  _node->eraseClient(i->get_id());
}

std::vector<Node::QueueDescription> Server::getDescription() const {
  return _node->getQueuesDescription();
}

std::vector<User> Server::users() const {
  return _users->users();
}

void Server::createQueue(const QueueSettings &settings) {
  _node->createQueue(settings, ServerID);
}

void Server::subscribe(const std::string &qname, EventConsumer *handler) {
  addHandler(qname, handler);
  _node->changeSubscription(Node::SubscribeActions::Subscribe, qname, ServerID);
}

void Server::unsubscribe(const std::string &qname) {
  _node->changeSubscription(Node::SubscribeActions::Unsubscribe, qname, ServerID);
}

void Server::publish(const std::string &qname, const rawData &data) {
  logger_info("server: publish to ", qname);
  _node->publish(qname, data, ServerID);
}
