#pragma once

#include <libdqueue/exports.h>
#include <libdqueue/kinds.h>
#include <libdqueue/network/abstract_client.h>
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

class Client : public network::AbstractClient, public utils::non_copy {
public:
  Client() = delete;
  EXPORT Client(boost::asio::io_service *service, const AbstractClient::Params &_params);
  EXPORT ~Client();

  EXPORT void connect();
  EXPORT void connectAsync();
  EXPORT void disconnect();
  EXPORT bool is_connected();

  EXPORT void waitAll() const;
  uint64_t getId() const { return _id;}

  EXPORT void onConnect() override;

  EXPORT void onNetworkError(const NetworkMessage_ptr &d,
                             const boost::system::error_code &err) override;

  EXPORT virtual void onMessage(const std::string & /*queueName*/,
                                const std::vector<uint8_t> & /*d*/){};

private:
  EXPORT void onNewMessage(const NetworkMessage_ptr &d, bool &cancel) override;
  void send(const NetworkMessage_ptr &nd);

  uint64_t getNextId() { return _nextMessageId++; }

  /*AsyncOperationResult makeNewQResult(uint64_t msgId) {
    auto qr = AsyncOperationResult::makeNew();
    _queries[msgId] = qr;
    qr.locker->lock();
    return qr;
  }*/

protected:
  mutable std::shared_mutex _locker;
  uint64_t _nextMessageId = 0;
  bool _loginConfirmed = false;
  uint64_t _id = 0;
  std::map<uint64_t, AsyncOperationResult> _queries;
};
} // namespace dqueue