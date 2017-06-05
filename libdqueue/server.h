#pragma once

#include <libdqueue/abstract_server.h>
#include <libdqueue/async_io.h>
#include <libdqueue/exports.h>
#include <libdqueue/utils/utils.h>
#include <mutex>

namespace dqueue {

class Server : public utils::non_copy {
public:
  EXPORT Server(boost::asio::io_service *service, AbstractServer::params &p);
  EXPORT virtual ~Server();
  EXPORT void serverStart();
  EXPORT void stopServer();
  EXPORT bool is_started();

protected:
  struct Private;
  std::shared_ptr<Private> _impl;
};
}
