#include <libdqueue/abstract_server.h>

#include <functional>
#include <string>

using namespace std::placeholders;
using namespace boost::asio;

using namespace dqueue;

AbstractServer::AbstractServer(boost::asio::io_service *service, AbstractServer::params p)
    : _service(service), _params(p) {
  _next_id.store(0);
}

AbstractServer::~AbstractServer() {
  _connections.clear();
}

void AbstractServer::serverStart() {
  boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), _params.port);
  auto new_socket = std::make_shared<boost::asio::ip::tcp::socket>(*_service);
  _acc = std::make_shared<boost::asio::ip::tcp::acceptor>(*_service, ep);
  start_accept(new_socket);
  is_started = true;
}

void AbstractServer::start_accept(socket_ptr sock) {
  _acc->async_accept(*sock, std::bind(&AbstractServer::handle_accept,
                                      this->shared_from_this(), sock, _1));
}

void AbstractServer::handle_accept(std::shared_ptr<AbstractServer> self, socket_ptr sock,
                                   const boost::system::error_code &err) {
  if (err) {
    if (err == boost::asio::error::operation_aborted ||
        err == boost::asio::error::connection_reset || err == boost::asio::error::eof) {
      return;
    } else {
      THROW_EXCEPTION("dariadb::server: error on accept - ", err.message());
    }
  } else {
    logger_info("server: accept connection.");

    self->_locker_connections.lock();
    auto new_client = std::make_shared<io>(self->_next_id, sock, self.get());
    self->_next_id++;
    self->_connections.push_back(new_client);
    self->_locker_connections.unlock();
  }
  socket_ptr new_sock = std::make_shared<boost::asio::ip::tcp::socket>(*self->_service);
  self->start_accept(new_sock);
}
