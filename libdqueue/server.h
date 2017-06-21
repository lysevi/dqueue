#pragma once

#include <libdqueue/abstract_server.h>
#include <libdqueue/async_io.h>
#include <libdqueue/exports.h>
#include <libdqueue/node.h>
#include <libdqueue/utils/utils.h>
#include <mutex>

namespace dqueue {

class Server : public utils::non_copy {
public:
  using dataHandler =
      std::function<void(const Queue &q, const std::vector<uint8_t> &data)>;
  EXPORT Server(boost::asio::io_service *service, AbstractServer::params &p);
  EXPORT virtual ~Server();
  EXPORT void serverStart();
  EXPORT void stopServer();
  EXPORT bool is_started();
  EXPORT std::vector<Node::QueueDescription> getDescription() const;
  EXPORT std::vector<Node::ClientDescription> getClientDescription() const;
  
  EXPORT void createQueue(const QueueSettings &settings);
  EXPORT void addDataHandler(Node::dataHandler dh);
  EXPORT void changeSubscription(Node::SubscribeActions action, std::string queueName);
protected:
  struct Private;
  std::shared_ptr<Private> _impl;
};
} // namespace dqueue
