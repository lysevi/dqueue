#include <libdqueue/abstract_client.h>

using namespace dqueue;

AbstractClient::AbstractClient(boost::asio::io_service *service, const Params &params)
    : _service(service), _params(params) {
  AsyncConnection::onDataRecvHandler on_d =
      [this](const NetworkMessage_ptr &d, bool &cancel) { this->onDataRecv(d, cancel); };
  AsyncConnection::onNetworkErrorHandler on_n =
      [this](const boost::system::error_code &err) { this->onNetworkError(err); };
  _async_connection = std::make_shared<AsyncConnection>(on_d, on_n);
}

AbstractClient::~AbstractClient() {
  disconnect();
}

void AbstractClient::disconnect() {
  if (!isStoped) {
    isStoped = true;
    _async_connection->full_stop();
  }
}

void AbstractClient::onNetworkError(const boost::system::error_code &err) {
  isConnected = false;
  if (!isStoped && _params.auto_reconnection) {
    this->async_connect();
  }
}

void AbstractClient::async_connect() {
  boost::asio::ip::tcp::resolver resolver(*_service);

  boost::asio::ip::tcp::resolver::query query(
      _params.host, std::to_string(_params.port),
      boost::asio::ip::tcp::resolver::query::canonical_name);
  boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);

  for (; iter != boost::asio::ip::tcp::resolver::iterator(); ++iter) {
    auto ep = iter->endpoint();
    if (ep.protocol() == boost::asio::ip::tcp::v4()) {
      break;
    }
  }
  if (iter == boost::asio::ip::tcp::resolver::iterator()) {
    THROW_EXCEPTION("hostname not found.");
  }
  boost::asio::ip::tcp::endpoint ep = *iter;
  logger_info("client: ", _params.host, ":", _params.port, " - ",
              ep.address().to_string());

  _socket = std::make_shared<boost::asio::ip::tcp::socket>(*_service);
  auto self = this->shared_from_this();
  _socket->async_connect(ep, [self](auto ec) {
    if (ec) {
		self->onNetworkError(ec);
    } else {
		self->_async_connection->start(self->_socket);
		self->isConnected = true;
		self->onConnect();
    }
  });
}

void AbstractClient::onDataRecv(const NetworkMessage_ptr &d, bool &cancel) {
  onNewMessage(d, cancel);
}
