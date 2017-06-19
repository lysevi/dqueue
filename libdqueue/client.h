#pragma once

#include <libdqueue/abstract_client.h>
#include <libdqueue/exports.h>
#include <libdqueue/q.h>
#include <libdqueue/utils/utils.h>
#include <boost/asio.hpp>

namespace dqueue {

class Client : public utils::non_copy {
public:
  using dataHandler = std::function<void(const Queue &q, const std::vector<uint8_t>&data)>;
  EXPORT Client(boost::asio::io_service *service, const AbstractClient::Params &_params);
  EXPORT ~Client();
  EXPORT void addHandler(dataHandler handler);
  EXPORT void asyncConnect();
  EXPORT void connect();
  EXPORT bool is_connected();
  EXPORT void createQueue(const QueueSettings &settings);
  EXPORT void subscribe(const std::string &qname);
  EXPORT void unsubscribe(const std::string &qname);
  EXPORT void publish(const std::string &qname, const std::vector<uint8_t> &data);
protected:
  struct Private;
  std::shared_ptr<Private> _impl;
};
} // namespace dqueue