#include <libdqueue/abstract_client.h>

using namespace dqueue;

abstract_client::abstract_client(boost::asio::io_service *service) : _service(service) {
  async_connection::onDataRecvHandler on_d =
      [this](const network_message_ptr &d, bool &cancel) { onDataRecv(d, cancel); };
  async_connection::onNetworkErrorHandler on_n =
      [this](const boost::system::error_code &err) { onNetworkError(err); };
  _async_connection = std::make_shared<async_connection>(on_d, on_n);
}

void abstract_client::onNetworkError(const boost::system::error_code &err) {
  THROW_EXCEPTION("error on - ", err.message());
}

void abstract_client::connectTo(const std::string &host, const std::string &port) {
  boost::asio::ip::tcp::resolver resolver(*_service);

  boost::asio::ip::tcp::resolver::query query(
      host, port, boost::asio::ip::tcp::resolver::query::canonical_name);
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
  logger_info("client: ", host, ":", port, " - ", ep.address().to_string());

  _socket = std::make_shared<boost::asio::ip::tcp::socket>(*_service);

  _socket->async_connect(ep, [this](auto ec) {
    if (ec) {
      auto msg = ec.message();
      THROW_EXCEPTION("dqueue::client: error on connect - ", msg);
    }
    this->_async_connection->start(this->_socket);

    logger_info("client: send hello ");

    auto nd = std::make_shared<network_message>(1);

    this->_async_connection->send(nd);
  });
}

void abstract_client::onDataRecv(const network_message_ptr &d, bool &cancel) {
  onNewMessage(d, cancel);
}
