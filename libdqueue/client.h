#pragma once

#include <libdqueue/abstract_client.h>
#include <libdqueue/exports.h>
#include <libdqueue/iqueue_client.h>
#include <libdqueue/kinds.h>
#include <libdqueue/memory_message_pool.h>
#include <libdqueue/node.h>
#include <libdqueue/q.h>
#include <libdqueue/queries.h>
#include <libdqueue/utils/utils.h>
#include <boost/asio.hpp>
#include <cstring>
#include <shared_mutex>

namespace dqueue {

class Client : public AbstractClient, public IQueueClient, public utils::non_copy {
public:
  Client() = delete;
  EXPORT Client(boost::asio::io_service *service, const AbstractClient::Params &_params);
  EXPORT ~Client();

  EXPORT void connect();
  EXPORT void connectAsync();
  EXPORT void disconnect();
  EXPORT bool is_connected();
  EXPORT Id getId() const;
  EXPORT size_t messagesInPool() const; // count dont sended messages.

  EXPORT void onConnect() override;
  EXPORT void onMessageSended(const NetworkMessage_ptr &d) override;

  EXPORT void onNetworkError(const NetworkMessage_ptr &d,
                             const boost::system::error_code &err) override;
  
  EXPORT virtual void onMessage(const std::string &queueName, const rawData &d){};

  EXPORT void addHandler(DataHandler handler) override;
  EXPORT void createQueue(const QueueSettings &settings) override;
  EXPORT void subscribe(const std::string &qname) override;
  EXPORT void unsubscribe(const std::string &qname) override;
  EXPORT void publish(const std::string &qname,
                      const std::vector<uint8_t> &data) override;

private:
  EXPORT void onNewMessage(const NetworkMessage_ptr &d, bool &cancel) override;
  void publish_inner(const queries::Publish &pb);
  void send(const NetworkMessage_ptr &nd);

protected:
  mutable std::shared_mutex _locker;
  uint64_t _nextMessageId = 0;
  DataHandler _handler;
  MessagePool_Ptr _messagePool;
  bool _loginConfirmed = false;
  Id _id;
};
} // namespace dqueue