#pragma once

#include <libdqueue/async_connection.h>
#include <libdqueue/exports.h>
#include <boost/asio.hpp>

namespace dqueue {

struct AbstractClient {
  struct Params {
    std::string host;
    unsigned short port;
    bool auto_reconnection = true;
  };

  std::shared_ptr<AsyncConnection> _async_connection = nullptr;
  boost::asio::io_service *_service = nullptr;
  socket_ptr _socket = nullptr;
  bool isConnected = false;
  bool isStoped = false;
  Params _params;

  EXPORT AbstractClient(boost::asio::io_service *service, const Params &_parms);
  EXPORT virtual ~AbstractClient();
  EXPORT void disconnect();
  EXPORT void async_connect();
  EXPORT void onNetworkError(const boost::system::error_code &err);
  EXPORT void onDataRecv(const NetworkMessage_ptr &d, bool &cancel);
  virtual void onConnect() = 0;
  virtual void onNewMessage(const NetworkMessage_ptr &d, bool &cancel) = 0;
};
}