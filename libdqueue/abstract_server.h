#pragma once

#include <libdqueue/async_connection.h>
#include <libdqueue/exports.h>
#include <mutex>

namespace dqueue {

struct AbstractServer {
  boost::asio::io_service *_service = nullptr;
  std::shared_ptr<boost::asio::ip::tcp::acceptor> _acc = nullptr;
  bool is_started = false;
  std::atomic_int _next_id;

  struct params {
    unsigned short port;
  };

  struct io {
    int id;
    socket_ptr sock = nullptr;
    std::shared_ptr<AsyncConnection> _async_connection = nullptr;

    AbstractServer *_server = nullptr;
    io(int id_, socket_ptr sock_, AbstractServer *s) : id(id_), sock(sock_), _server(s) {

      AsyncConnection::onDataRecvHandler on_d =
          [this](const NetworkMessage_ptr &d, bool &cancel) { onDataRecv(d, cancel); };
      AsyncConnection::onNetworkErrorHandler on_n =
          [this](const boost::system::error_code &err) { onNetworkError(err); };

	  _async_connection = std::make_shared<AsyncConnection>(on_d, on_n);
      _async_connection->set_id(id);
      _async_connection->start(sock);
    }
    ~io() {
      if (_async_connection != nullptr) {
        _async_connection->full_stop();
        _async_connection = nullptr;
      }
    }

    void onNetworkError(const boost::system::error_code &err) {
      this->_server->onNetworkError(err);
    }

    void onDataRecv(const NetworkMessage_ptr &d, bool &cancel) {
      _server->onNewMessage(*this, d, cancel);
    }
  };

  std::mutex _locker_connections;
  std::list<std::shared_ptr<io>> _connections;
  params _params;

  EXPORT AbstractServer(boost::asio::io_service *service, params p);
  EXPORT virtual ~AbstractServer();
  EXPORT void serverStart();

  EXPORT void start_accept(socket_ptr sock);
  EXPORT void handle_accept(socket_ptr sock, const boost::system::error_code &err);

  EXPORT virtual void onNetworkError(const boost::system::error_code &err) = 0;
  EXPORT virtual void onNewMessage(io &i, const NetworkMessage_ptr &d, bool &cancel) = 0;
};
}