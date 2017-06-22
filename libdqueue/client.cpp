#include <libdqueue/client.h>
#include <libdqueue/kinds.h>
#include <libdqueue/memory_message_pool.h>
#include <libdqueue/queries.h>
#include <cstring>
#include <shared_mutex>

using namespace dqueue;

struct Client::Private final : virtual public AbstractClient, public IQueueClient {

  Private(boost::asio::io_service *service, const AbstractClient::Params &_params)
      : AbstractClient(service, _params) {
    _messagePool = std::make_shared<MemoryMessagePool>();
  }

  virtual ~Private() {}

  void connect() {
    this->async_connect();
    while (!this->is_connected()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
  }

  void onConnect() override {
    auto all = _messagePool->all();
    for (auto p : all) {
      this->publish_inner(p);
    }
  }

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
    case (NetworkMessage::message_kind)MessageKinds::OK: {
      logger_info("client: recv ok");
      auto cs = queries::Ok(d);
      _messagePool->erase(cs.id);
      break;
    }
    default:
      THROW_EXCEPTION("unknow message kind: ", hdr->kind);
    }
  }

  void onNetworkError(const NetworkMessage_ptr &d,
                      const boost::system::error_code &err) override {}

  void addHandler(DataHandler handler) override { _handler = handler; }

  size_t messagesInPool() const { return _messagePool->size(); }

  void createQueue(const QueueSettings &settings) override {
    logger_info("client: createQueue ", settings.name);
    queries::CreateQueue cq(settings.name);
    auto nd = cq.toNetworkMessage();
    send(nd);
  }

  void subscribe(const std::string &qname) override {
    logger_info("client: subscribe ", qname);

    queries::ChangeSubscribe cs(qname);

    auto nd = cs.toNetworkMessage();
    nd->cast_to_header()->kind =
        static_cast<NetworkMessage::message_kind>(MessageKinds::SUBSCRIBE);

    send(nd);
  }

  void unsubscribe(const std::string &qname) override {
    logger_info("client: unsubscribe ", qname);
    queries::ChangeSubscribe cs(qname);

    auto nd = cs.toNetworkMessage();
    nd->cast_to_header()->kind =
        static_cast<NetworkMessage::message_kind>(MessageKinds::UNSUBSCRIBE);

    send(nd);
  }

  void publish(const std::string &qname, const std::vector<uint8_t> &data) override {
    std::lock_guard<std::shared_mutex> lg(_locker);
    queries::Publish pb(qname, data, _nextMessageId++);
    _messagePool->append(pb);

    publish_inner(pb);
  }

  void publish_inner(const queries::Publish &pb) {
    logger_info("client: publish ", pb.qname);
    auto nd = pb.toNetworkMessage();
    send(nd);
  }

  void send(const NetworkMessage_ptr &nd) {
    if (_async_connection != nullptr) {
      _async_connection->send(nd);
    }
  }

  mutable std::shared_mutex _locker;
  uint64_t _nextMessageId = 0;
  DataHandler _handler;
  MessagePool_Ptr _messagePool;
};

Client::Client(boost::asio::io_service *service, const AbstractClient::Params &_params)
    : _impl(std::make_shared<Private>(service, _params)) {}

Client::~Client() {
  _impl = nullptr;
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

size_t Client::messagesInPool() const {
  return _impl->messagesInPool();
}

void Client::addHandler(DataHandler handler) {
  return _impl->addHandler(handler);
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
