#pragma once

#include <libdqueue/exports.h>
#include <libdqueue/iqueue_client.h>
#include <libdqueue/network/abstract_server.h>
#include <libdqueue/network/async_io.h>
#include <libdqueue/node.h>
#include <libdqueue/node_settings.h>
#include <libdqueue/queries.h>
#include <libdqueue/users.h>
#include <libdqueue/utils/utils.h>

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

namespace dqueue {

class Server : public network::AbstractServer,
               public utils::non_copy,
               public IQueueClient {
public:
  EXPORT Server(boost::asio::io_service *service, network::AbstractServer::Params &p);
  EXPORT virtual ~Server();
  /*EXPORT void serverStart();
  EXPORT void stopServer();
  EXPORT bool is_started();*/
  EXPORT std::vector<Node::QueueDescription> getDescription() const;
  EXPORT std::vector<User> users() const;

  EXPORT void onStartComplete() override;
  EXPORT network::ON_NEW_CONNECTION_RESULT
  onNewConnection(ClientConnection_Ptr i) override;
  EXPORT void createQueue(const QueueSettings &settings,
                          const OperationType ot = OperationType::Sync) override;
  EXPORT void subscribe(const SubscriptionParams &settings, EventConsumer *handler,
                        const OperationType ot = OperationType::Sync) override;
  EXPORT void unsubscribe(const std::string &qname,
                          const OperationType ot = OperationType::Sync) override;
  EXPORT void publish(const PublishParams &settings, const std::vector<uint8_t> &data,
                      const OperationType ot = OperationType::Sync) override;

  EXPORT void onNetworkError(ClientConnection_Ptr i, const NetworkMessage_ptr &d,
                             const boost::system::error_code &err) override;
  EXPORT void onNewMessage(ClientConnection_Ptr i, const NetworkMessage_ptr &d,
                           bool &cancel) override;
  EXPORT void onDisconnect(const AbstractServer::ClientConnection_Ptr &i) override;

protected:
  void onSendToClient(const PublishParams &info, const std::vector<uint8_t> &rd, Id id);

  void sendOk(ClientConnection_Ptr i, uint64_t messageId);

protected:
  std::mutex _locker;
  uint64_t _nextMessageId = 0;
  std::unique_ptr<Node> _node;
  UserBase_Ptr _users;
};
} // namespace dqueue
