#pragma once

#include <libdqueue/async_io.h>
#include <libdqueue/exports.h>
#include <boost/asio.hpp>

namespace dqueue {

class AbstractClient : public std::enable_shared_from_this<AbstractClient> {
public:
  struct Params {
    std::string host;
    unsigned short port;
    bool auto_reconnection = true;
  };

  EXPORT AbstractClient(boost::asio::io_service *service, const Params &_parms);
  EXPORT virtual ~AbstractClient();
  EXPORT void disconnect();
  EXPORT void async_connect();
  EXPORT void reconnectOnError(const NetworkMessage_ptr &d,
                               const boost::system::error_code &err);
  EXPORT void dataRecv(const NetworkMessage_ptr &d, bool &cancel);

  virtual void onConnect() = 0;
  virtual void onMessageSended(const NetworkMessage_ptr &d) = 0;
  virtual void onNewMessage(const NetworkMessage_ptr &d, bool &cancel) = 0;
  virtual void onNetworkError(const NetworkMessage_ptr &d,
                              const boost::system::error_code &err) = 0;

  EXPORT bool is_connected() const { return isConnected; }
  EXPORT bool is_stoped() const { return isStoped; }

protected:
  std::shared_ptr<AsyncIO> _async_connection = nullptr;
  boost::asio::io_service *_service = nullptr;
  socket_ptr _socket = nullptr;
  bool isConnected = false;
  bool isStoped = false;
  Params _params;
};
}