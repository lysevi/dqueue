#include <libdqueue/abstract_client.h>

using namespace dqueue;

AbstractClient::AbstractClient(boost::asio::io_service *service, const Params &params)
    : _service(service), _params(params) {}

AbstractClient::~AbstractClient() {
  disconnect();
}

void AbstractClient::disconnect() {
  if (!isStoped) {
    isStoped = true;
    _async_connection->full_stop();
  }
}

void AbstractClient::reconnectOnError(const NetworkMessage_ptr &d,
                                      const boost::system::error_code &err) {
  isConnected = false;
  onNetworkError(d, err);
  if (!isStoped && _params.auto_reconnection) {
    this->async_connect();
  }
}

void AbstractClient::async_connect() {
  using namespace boost::asio::ip;
  tcp::resolver resolver(*_service);
  tcp::resolver::query query(_params.host, std::to_string(_params.port),
                             tcp::resolver::query::canonical_name);
  tcp::resolver::iterator iter = resolver.resolve(query);

  for (; iter != tcp::resolver::iterator(); ++iter) {
    auto ep = iter->endpoint();
    if (ep.protocol() == tcp::v4()) {
      break;
    }
  }
  if (iter == tcp::resolver::iterator()) {
    THROW_EXCEPTION("hostname not found.");
  }
  tcp::endpoint ep = *iter;
  logger_info("client(", _params.login, "): start async connection to ", _params.host,
              ":", _params.port, " - ", ep.address().to_string());

  _socket = std::make_shared<boost::asio::ip::tcp::socket>(*_service);
  auto self = this->shared_from_this();
  _socket->async_connect(ep, [self](auto ec) {
    if (ec) {
      self->reconnectOnError(nullptr, ec);
    } else {
      logger_info("client(", self->_params.login, "): connected.");
      AsyncIO::onDataRecvHandler on_d = [self](auto d, auto cancel) {
        self->dataRecv(d, cancel);
      };
      AsyncIO::onNetworkErrorHandler on_n = [self](auto d, auto err) {
        self->reconnectOnError(d, err);
      };

      self->_async_connection = std::make_shared<AsyncIO>(on_d, on_n);
      self->_async_connection->start(self->_socket);
      self->isConnected = true;
      self->onConnect();
    }
  });
}

void AbstractClient::dataRecv(const NetworkMessage_ptr &d, bool &cancel) {
  onNewMessage(d, cancel);
}
