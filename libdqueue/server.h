#pragma once

#include <libdqueue/abstract_server.h>
#include <libdqueue/async_io.h>
#include <libdqueue/exports.h>
#include <libdqueue/iqueue_client.h>
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

class Server : public AbstractServer, public utils::non_copy, public IQueueClient {
public:
  EXPORT Server(boost::asio::io_service *service, AbstractServer::params &p);
  EXPORT virtual ~Server();
  /*EXPORT void serverStart();
  EXPORT void stopServer();
  EXPORT bool is_started();*/
  EXPORT std::vector<Node::QueueDescription> getDescription() const;
  EXPORT std::vector<User> users() const;

  EXPORT ON_NEW_CONNECTION_RESULT onNewConnection(ClientConnection_Ptr i) override;
  EXPORT void createQueue(const QueueSettings &settings,
                          const OperationType ot = OperationType::Sync) override;
  EXPORT void subscribe(const SubscriptionParams &settings, EventConsumer *handler,
                        const OperationType ot = OperationType::Sync) override;
  EXPORT void unsubscribe(const std::string &qname,
                          const OperationType ot = OperationType::Sync) override;
  EXPORT void publish(const PublishParams &settings, const rawData &data) override;

private:
  void onMessageSended(ClientConnection_Ptr i, const NetworkMessage_ptr &d) override;
  void onNetworkError(ClientConnection_Ptr i, const NetworkMessage_ptr &d,
                      const boost::system::error_code &err) override;
  void onSendToClient(const PublishParams &info, const rawData &rd, Id id);
  void onNewMessage(ClientConnection_Ptr i, const NetworkMessage_ptr &d, bool &cancel);

  void sendOk(ClientConnection_Ptr i, uint64_t messageId);
  void onDisconnect(const AbstractServer::ClientConnection_Ptr &i) override;

protected:
  std::mutex _locker;
  uint64_t _nextMessageId = 0;
  std::unique_ptr<Node> _node;
  UserBase_Ptr _users;
};
} // namespace dqueue
