#include <libdqueue/client.h>
#include <libdqueue/kinds.h>
#include <libdqueue/queries.h>
#include <cstring>

using namespace dqueue;

struct Client::Private final : virtual public AbstractClient {

  Private(boost::asio::io_service *service, const AbstractClient::Params &_params)
      : AbstractClient(service, _params) {}

  virtual ~Private() {}

  void addHandler(Node::dataHandler handler) { _handler = handler; }

  void connect() {
    this->async_connect();
    while (!this->is_connected()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
  }

  void onConnect() override {}

  void onMessageSended(const NetworkMessage_ptr &d) override {}

  void onNewMessage(const NetworkMessage_ptr &d, bool &cancel) override {
    auto hdr = d->cast_to_header();

    switch (hdr->kind) {
    case (NetworkMessage::message_kind)MessageKinds::PUBLISH: {
      logger_info("client: recv publish");
      auto cs = queries::Publish(d);
      if (this->_handler != nullptr) {
        _handler(cs.qname, cs.data, 0);
      } else {
        logger_info("client: _handler was not be set");
      }
      break;
    }
    default:
      THROW_EXCEPTION("unknow message kind: ", hdr->kind);
    }
  }

  void onNetworkError(const NetworkMessage_ptr &d,
                      const boost::system::error_code &err) override {}

  void createQueue(const QueueSettings &settings) {
    logger_info("client: createQueue ", settings.name);
    queries::CreateQueue cq(settings.name);
    auto nd = cq.toNetworkMessage();
    this->_async_connection->send(nd);
  }

  void subscribe(const std::string &qname) {
    logger_info("client: subscribe ", qname);

    queries::ChangeSubscribe cs(qname);

    auto nd = cs.toNetworkMessage();
    nd->cast_to_header()->kind =
        static_cast<NetworkMessage::message_kind>(MessageKinds::SUBSCRIBE);

    this->_async_connection->send(nd);
  }

  void unsubscribe(const std::string &qname) {
    logger_info("client: unsubscribe ", qname);
    queries::ChangeSubscribe cs(qname);

    auto nd = cs.toNetworkMessage();
    nd->cast_to_header()->kind =
        static_cast<NetworkMessage::message_kind>(MessageKinds::UNSUBSCRIBE);

    this->_async_connection->send(nd);
  }

  void publish(const std::string &qname, const std::vector<uint8_t> &data) {
    logger_info("client: publish ", qname);
    queries::Publish pb(qname, data);
    auto nd = pb.toNetworkMessage();
    this->_async_connection->send(nd);
  }

  Node::dataHandler _handler;
};

Client::Client(boost::asio::io_service *service, const AbstractClient::Params &_params)
    : _impl(std::make_shared<Private>(service, _params)) {}

Client::~Client() {
  _impl = nullptr;
}

void Client::addHandler(Node::dataHandler handler) {
  return _impl->addHandler(handler);
}

void Client::asyncConnect() {
  _impl->async_connect();
}

bool Client::is_connected() {
  return _impl->is_connected();
}

void Client::connect() {
  return _impl->connect();
}

void Client::disconnect() {
  return _impl->disconnect();
}

void Client::createQueue(const QueueSettings &settings) {
  _impl->createQueue(settings);
}

void Client::subscribe(const std::string &qname) {
  return _impl->subscribe(qname);
}

void Client::unsubscribe(const std::string &qname) {
  return _impl->unsubscribe(qname);
}

void Client::publish(const std::string &qname, const std::vector<uint8_t> &data) {
  return _impl->publish(qname, data);
}
