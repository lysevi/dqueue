#include <libdqueue/abstract_server.h>
#include <libdqueue/utils/utils.h>
#include <functional>
#include <string>

using namespace std::placeholders;
using namespace boost::asio;
using namespace boost::asio::ip;

using namespace dqueue;

AbstractServer::ClientConnection::ClientConnection(int id_, socket_ptr sock_,
                                                   std::shared_ptr<AbstractServer> s)
    : id(id_), sock(sock_), _server(s) {}

AbstractServer::ClientConnection::~ClientConnection() {}

void AbstractServer::ClientConnection::start() {
  auto self = shared_from_this();
  AsyncIO::onDataRecvHandler on_d = [self](const NetworkMessage_ptr &d, bool &cancel) {
    self->onDataRecv(d, cancel);
  };

  AsyncIO::onNetworkErrorHandler on_n = [self](auto d, auto err) {
    self->onNetworkError(d, err);
    self->close();
  };

  AsyncIO::onNetworkSuccessSendHandler on_s = [self](auto d) {
    self->onMessageSended(d);
  };

  _async_connection = std::make_shared<AsyncIO>(on_d, on_n, on_s);
  _async_connection->set_id(id);
  _async_connection->start(sock);
}

void AbstractServer::ClientConnection::close() {
  if (_async_connection != nullptr) {
    _async_connection->full_stop();
    _async_connection = nullptr;

    this->_server->erase_client_description(this);
  }
}

void AbstractServer::ClientConnection::onMessageSended(const NetworkMessage_ptr &d) {
  this->_server->onMessageSended(*this, d);
}
void AbstractServer::ClientConnection::onNetworkError(
    const NetworkMessage_ptr &d, const boost::system::error_code &err) {
  this->_server->onNetworkError(*this, d, err);
}

void AbstractServer::ClientConnection::onDataRecv(const NetworkMessage_ptr &d,
                                                  bool &cancel) {
  _server->onNewMessage(*this, d, cancel);
}

void AbstractServer::ClientConnection::sendData(const NetworkMessage_ptr &d) {
  _async_connection->send(d);
}

/////////////////////////////////////////////////////////////////

AbstractServer::AbstractServer(boost::asio::io_service *service, AbstractServer::params p)
    : _service(service), _params(p) {
  _next_id.store(0);
}

AbstractServer::~AbstractServer() {
  stopServer();
}

void AbstractServer::serverStart() {
  tcp::endpoint ep(tcp::v4(), _params.port);
  auto new_socket = std::make_shared<boost::asio::ip::tcp::socket>(*_service);
  _acc = std::make_shared<boost::asio::ip::tcp::acceptor>(*_service, ep);
  start_accept(new_socket);
  _is_started = true;
}

void AbstractServer::start_accept(socket_ptr sock) {
  _acc->async_accept(*sock,
                     std::bind(&handle_accept, this->shared_from_this(), sock, _1));
}

void AbstractServer::erase_client_description(const ClientConnection *client) {
  std::lock_guard<std::mutex> lg(_locker_connections);
  auto it = std::find_if(_connections.begin(), _connections.end(),
                         [client](auto c) { return c->get_id() == client->get_id(); });
  ENSURE(it != _connections.end());
  onDisconnect(*client);
  _connections.erase(it);
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
    std::shared_ptr<ClientConnection> new_client = nullptr;
    {
      std::lock_guard<std::mutex> lg(self->_locker_connections);
      new_client =
          std::make_shared<AbstractServer::ClientConnection>(self->_next_id, sock, self);
      self->_next_id++;
    }

    if (self->onNewConnection(*new_client.get()) == ON_NEW_CONNECTION_RESULT::ACCEPT) {
      std::lock_guard<std::mutex> lg(self->_locker_connections);
      new_client->start();
      self->_connections.push_back(new_client);
    }
  }
  socket_ptr new_sock = std::make_shared<boost::asio::ip::tcp::socket>(*self->_service);
  self->start_accept(new_sock);
}

void AbstractServer::stopServer() {
  if (!_is_stoped) {
    logger("abstract_server::stopServer()");
    _acc = nullptr;
    if (!_connections.empty()) {
      std::vector<std::shared_ptr<ClientConnection>> local_copy(_connections.begin(),
                                                                _connections.end());
      for (auto con : local_copy) {
        con->close();
      }
      _connections.clear();
    }
    _is_stoped = true;
  }
}

void AbstractServer::sendTo(int id, NetworkMessage_ptr &d) {
  std::lock_guard<std::mutex> lg(this->_locker_connections);
  for (auto c : _connections) {
    if (c->get_id() == id) {
      c->sendData(d);
      return;
    }
  }
  THROW_EXCEPTION("server: unknow client #", id);
}
