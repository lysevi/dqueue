#include <libdqueue/client.h>

using namespace dqueue;

Client::Client(boost::asio::io_service *service, const AbstractClient::Params &_params)
    : AbstractClient(service, _params) {
  _messagePool = std::make_shared<MemoryMessagePool>();
}

Client::~Client() {}

bool Client::is_connected() {
  return _loginConfirmed;
}

void Client::connect() {
  this->async_connect();
  while (!this->is_connected()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }
}

void Client::connectAsync() {
  this->async_connect();
}

void Client::disconnect() {
  return AbstractClient::disconnect();
}

void Client::onConnect() {
  queries::Login lg(this->_params.login);
  _loginConfirmed = false;
  this->send(lg.toNetworkMessage());

  auto all = _messagePool->all();
  for (auto p : all) {
    this->publish_inner(p);
  }
}

void Client::onMessageSended(const NetworkMessage_ptr &d) {}

void Client::onNewMessage(const NetworkMessage_ptr &d, bool &cancel) {
  auto hdr = d->cast_to_header();

  switch (hdr->kind) {
  case (NetworkMessage::message_kind)MessageKinds::PUBLISH: {
    logger_info("client (", _params.login, "): recv publish");
    auto cs = queries::Publish(d);
    this->callConsumer(cs.toPublishParams(), cs.data, _id);
    onMessage(cs.qname, cs.data);
    break;
  }
  case (NetworkMessage::message_kind)MessageKinds::OK: {
    logger_info("client (", _params.login, "): recv ok");
    auto cs = queries::Ok(d);
    _messagePool->erase(cs.id);
    break;
  }

  case (NetworkMessage::message_kind)MessageKinds::LOGIN_CONFIRM: {
    logger_info("client (", _params.login, "): login confirm");
    auto lc = queries::LoginConfirm(d);
    _id = lc.id;
    _loginConfirmed = true;
    break;
  }
  default:
    THROW_EXCEPTION("client (", _params.login, "):unknow message kind: ", hdr->kind);
  }
}

void Client::onNetworkError(const NetworkMessage_ptr &d,
                            const boost::system::error_code &err) {
  bool isError = err == boost::asio::error::operation_aborted ||
                 err == boost::asio::error::connection_reset ||
                 err == boost::asio::error::eof;
  if (isError && !isStoped) {
    int errCode = err.value();
    std::string msg = err.message();
    logger_fatal("client (", _params.login, "): network error (", errCode, ") - ", msg);
  }
  _loginConfirmed = false;
}

size_t Client::messagesInPool() const {
  return _messagePool->size();
}

Id Client::getId() const {
  return _id;
}

void Client::createQueue(const QueueSettings &settings) {
  logger_info("client (", _params.login, "): createQueue ", settings.name);
  queries::CreateQueue cq(settings.name);
  auto nd = cq.toNetworkMessage();
  send(nd);
}

void Client::subscribe(const SubscriptionParams &settings, EventConsumer *handler) {
  logger_info("client (", _params.login, "): subscribe ", settings.queueName);
  this->addHandler(settings, handler);
  queries::ChangeSubscribe cs(settings);
  auto nd = cs.toNetworkMessage();
  nd->cast_to_header()->kind =
      static_cast<NetworkMessage::message_kind>(MessageKinds::SUBSCRIBE);

  send(nd);
}

void Client::unsubscribe(const std::string &qname) {
  logger_info("client (", _params.login, "): unsubscribe ", qname);
  SubscriptionParams settings{qname};
  queries::ChangeSubscribe cs(settings);

  auto nd = cs.toNetworkMessage();
  nd->cast_to_header()->kind =
      static_cast<NetworkMessage::message_kind>(MessageKinds::UNSUBSCRIBE);

  send(nd);
}

void Client::publish(const PublishParams &settings, const std::vector<uint8_t> &data) {
  std::lock_guard<std::shared_mutex> lg(_locker);
  queries::Publish pb(settings, data, _nextMessageId++);
  _messagePool->append(pb);

  publish_inner(pb);
}

void Client::publish_inner(const queries::Publish &pb) {
  logger_info("client (", _params.login, "): publish ", pb.qname);
  auto nd = pb.toNetworkMessage();
  send(nd);
}

void Client::send(const NetworkMessage_ptr &nd) {
  if (_async_connection != nullptr) {
    _async_connection->send(nd);
  }
}