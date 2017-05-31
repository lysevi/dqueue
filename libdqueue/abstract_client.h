#pragma once

#include <libdqueue/async_connection.h>
#include <libdqueue/exports.h>
#include <boost/asio.hpp>

namespace dqueue { 

struct abstract_client {
  std::shared_ptr<async_connection> _async_connection = nullptr;
  boost::asio::io_service *_service = nullptr;
  socket_ptr _socket = nullptr;

  EXPORT abstract_client(boost::asio::io_service *service);
  EXPORT ~abstract_client();
  EXPORT void connectTo(const std::string &host, const std::string &port);
  EXPORT void onNetworkError(const boost::system::error_code &err);
  EXPORT void onDataRecv(const network_message_ptr &d, bool &cancel);
  virtual void onNewMessage(const network_message_ptr &d, bool &cancel) = 0;
};
}