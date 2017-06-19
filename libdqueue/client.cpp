#include <libdqueue/client.h>
#include <libdqueue/kinds.h>

using namespace dqueue;

struct Client::Private final : virtual public AbstractClient {

  Private(boost::asio::io_service *service, const AbstractClient::Params &_params)
      : AbstractClient(service, _params) {}

  virtual ~Private() {}

  void connect() {
    this->async_connect();
    while (!this->isConnected) {
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
  }

  void onConnect() override {}

  void onMessageSended(const NetworkMessage_ptr &d) override {}

  void onNewMessage(const NetworkMessage_ptr &d, bool &cancel) override {}

  void onNetworkError(const NetworkMessage_ptr &d,
                      const boost::system::error_code &err) override {}

  void createQueue(const QueueSettings &settings) {
    logger_info("client: createQueue ", settings.name);
    auto nd = settings.toNetworkMessage();
    nd->cast_to_header()->kind =
        static_cast<NetworkMessage::message_kind>(MessageKinds::CREATE_QUEUE);
    this->_async_connection->send(nd);
  }
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

void Client::createQueue(const QueueSettings &settings) {
  _impl->createQueue(settings);
}