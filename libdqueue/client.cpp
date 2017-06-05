#include <libdqueue/client.h>

using namespace dqueue;

struct Client::Private final : virtual public AbstractClient {

  Private(boost::asio::io_service *service, const AbstractClient::Params &_params)
      : AbstractClient(service, _params) {}

  virtual ~Private() {}

  void onConnect() override {}

  void onMessageSended(const NetworkMessage_ptr &d) override {}

  void onNewMessage(const NetworkMessage_ptr &d, bool &cancel) override {}

  void onNetworkError(const NetworkMessage_ptr &d,
                      const boost::system::error_code &err) override {}
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