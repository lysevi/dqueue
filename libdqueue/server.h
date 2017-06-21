#pragma once

#include <libdqueue/abstract_server.h>
#include <libdqueue/async_io.h>
#include <libdqueue/exports.h>
#include <libdqueue/iqueue_client.h>
#include <libdqueue/node.h>
#include <libdqueue/utils/utils.h>
#include <mutex>

namespace dqueue {

class Server : public utils::non_copy, public IQueueClient {
public:
  EXPORT Server(boost::asio::io_service *service, AbstractServer::params &p);
  EXPORT virtual ~Server();
  EXPORT void serverStart();
  EXPORT void stopServer();
  EXPORT bool is_started();
  EXPORT std::vector<Node::QueueDescription> getDescription() const;
  EXPORT std::vector<User> users()const;

  EXPORT void addHandler(DataHandler handler) override;
  EXPORT void createQueue(const QueueSettings &settings) override;
  EXPORT void subscribe(const std::string &qname) override;
  EXPORT void unsubscribe(const std::string &qname) override;
  EXPORT void publish(const std::string &qname, const rawData &data) override;

protected:
  struct Private;
  std::shared_ptr<Private> _impl;
};
} // namespace dqueue
