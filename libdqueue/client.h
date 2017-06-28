#pragma once

#include <libdqueue/abstract_client.h>
#include <libdqueue/exports.h>
#include <libdqueue/iqueue_client.h>
#include <libdqueue/kinds.h>
#include <libdqueue/memory_message_pool.h>
#include <libdqueue/node.h>
#include <libdqueue/q.h>
#include <libdqueue/queries.h>
#include <libdqueue/utils/async/locker.h>
#include <libdqueue/utils/utils.h>
#include <boost/asio.hpp>
#include <cstring>
#include <shared_mutex>

namespace dqueue {

struct AsyncOperationResult {
  std::shared_ptr<utils::async::locker> locker;
  static AsyncOperationResult makeNew() {
    AsyncOperationResult result;
    result.locker = std::make_shared<utils::async::locker>();
    return result;
  }
};

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

  EXPORT virtual void onMessage(const std::string & /*queueName*/,
                                const rawData & /*d*/){};

  EXPORT void createQueue(const QueueSettings &settings,
                          const OperationType ot = OperationType::Sync) override;
  EXPORT void subscribe(const SubscriptionParams &settings, EventConsumer *handler,
                        const OperationType ot = OperationType::Sync) override;
  EXPORT void unsubscribe(const std::string &qname,
                          const OperationType ot = OperationType::Sync) override;
  EXPORT void publish(const PublishParams &settings, const std::vector<uint8_t> &data,
                      const OperationType ot = OperationType::Sync) override;

private:
  EXPORT void onNewMessage(const NetworkMessage_ptr &d, bool &cancel) override;
  void publish_inner(const queries::Publish &pb);
  void send(const NetworkMessage_ptr &nd);

  uint64_t getNextId() { return _nextMessageId++; }

  AsyncOperationResult makeNewQResult(uint64_t msgId) {
    auto qr = AsyncOperationResult::makeNew();
    _queries[msgId] = qr;
    qr.locker->lock();
    return qr;
  }

protected:
  mutable std::shared_mutex _locker;
  uint64_t _nextMessageId = 0;
  MessagePool_Ptr _messagePool;
  bool _loginConfirmed = false;
  Id _id;
  std::map<uint64_t, AsyncOperationResult> _queries;
};
} // namespace dqueue