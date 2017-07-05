#include <libdqueue/node.h>
#include <libdqueue/node_settings.h>
#include <libdqueue/queries.h>
#include <libdqueue/server.h>
#include <libdqueue/users.h>

#include <functional>
#include <string>

using namespace dqueue;

Server::Server(boost::asio::io_service *service, AbstractServer::Params &p)
    : AbstractServer(service, p) {
  DataHandler dhandler = [this](const PublishParams &info, const std::vector<uint8_t> &rd, Id id) {
    this->onSendToClient(info, rd, id);
  };
  _users = UserBase::create();
  _node = std::make_unique<Node>(Node::Settings(), dhandler, _users);
  _users->append({"server", ServerID});
}

Server::~Server() {
  _node->eraseClient(ServerID);
}

void Server::onNetworkError(ClientConnection_Ptr i, const NetworkMessage_ptr &d,
                            const boost::system::error_code &err) {
  bool operation_aborted = err == boost::asio::error::operation_aborted;
  bool eof = err == boost::asio::error::eof;
  if (!operation_aborted && !eof) {
    std::string errm = err.message();
    logger_fatal("server: ", errm);
  }
}

void Server::onSendToClient(const PublishParams &info, const std::vector<uint8_t> &rd, Id id) {
  if (id == ServerID) {
    this->callConsumer(info, rd, id);
  } else {
    std::lock_guard<std::mutex> lg(_locker);
    // TODO make one allocation in onNewMessage
    queries::Publish pub(info, rd, _nextMessageId++);
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
    sendOk(i, cq.msgId);
    break;
  }
  case (NetworkMessage::message_kind)MessageKinds::SUBSCRIBE: {
    logger_info("server: #", i->get_id(), " subscribe");
    auto cs = queries::ChangeSubscribe(d);
    _node->changeSubscription(SubscribeActions::Subscribe, cs.toParams(), i->get_id());
    sendOk(i, cs.msgId);
    break;
  }
  case (NetworkMessage::message_kind)MessageKinds::UNSUBSCRIBE: {
    logger_info("server: #", i->get_id(), " unsubscribe");
    auto cs = queries::ChangeSubscribe(d);
    _node->changeSubscription(SubscribeActions::Unsubscribe, cs.toParams(), i->get_id());
    sendOk(i, cs.msgId);
    break;
  }
  case (NetworkMessage::message_kind)MessageKinds::PUBLISH: {
    logger("server: #", i->get_id(), " publish");
    auto cs = queries::Publish(d);
    PublishParams param = cs.toPublishParams();
    _node->publish(param, cs.data, i->get_id());
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

void Server::onStartComplete() {
  logger_info("server started.");
}

ON_NEW_CONNECTION_RESULT Server::onNewConnection(ClientConnection_Ptr i) {
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

void Server::createQueue(const QueueSettings &settings, const OperationType) {
  _node->createQueue(settings, ServerID);
}

void Server::subscribe(const SubscriptionParams &settings, EventConsumer *handler,
                       const OperationType) {
  addHandler(settings, handler);
  _node->changeSubscription(SubscribeActions::Subscribe, settings, ServerID);
}

void Server::unsubscribe(const std::string &qname, const OperationType) {
  SubscriptionParams ss(qname);
  _node->changeSubscription(SubscribeActions::Subscribe, ss, ServerID);
}

void Server::publish(const PublishParams &settings, const std::vector<uint8_t> &data,
                     const OperationType ot) {
  logger_info("server: publish to ", settings.queueName);
  _node->publish(settings, data, ServerID);
}
