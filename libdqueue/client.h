#pragma once

#include <libdqueue/abstract_client.h>
#include <libdqueue/exports.h>
#include <libdqueue/iqueue_client.h>
#include <libdqueue/node.h>
#include <libdqueue/q.h>
#include <libdqueue/utils/utils.h>
#include <boost/asio.hpp>

namespace dqueue {

class Client : public utils::non_copy, public IQueueClient {
public:
  EXPORT Client(boost::asio::io_service *service, const AbstractClient::Params &_params);
  EXPORT ~Client();

  EXPORT void asyncConnect();
  EXPORT void connect();
  EXPORT void disconnect();
  EXPORT bool is_connected();
  EXPORT size_t messagesInPool()const; // count dont sended messages.

  EXPORT void addHandler(DataHandler handler) override;
  EXPORT void createQueue(const QueueSettings &settings) override;
  EXPORT void subscribe(const std::string &qname) override;
  EXPORT void unsubscribe(const std::string &qname) override;
  EXPORT void publish(const std::string &qname,
                      const std::vector<uint8_t> &data) override;
protected:
  struct Private;
  std::shared_ptr<Private> _impl;
};
} // namespace dqueue