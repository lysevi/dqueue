#pragma once

#include <libdqueue/abstract_client.h>
#include <libdqueue/exports.h>
#include <libdqueue/utils/utils.h>
#include <libdqueue/q.h>
#include <boost/asio.hpp>

namespace dqueue {

class Client : public utils::non_copy {
public:
  EXPORT Client(boost::asio::io_service *service, const AbstractClient::Params &_params);
  EXPORT ~Client();
  EXPORT void asyncConnect();
  EXPORT void connect();
  EXPORT bool is_connected();
  EXPORT void createQueue(const QueueSettings&settings);
protected:
  struct Private;
  std::shared_ptr<Private> _impl;
};
}