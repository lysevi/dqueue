#pragma once

#include <libdqueue/exports.h>
#include <libdqueue/q.h>
#include <libdqueue/users.h>
#include <libdqueue/utils/utils.h>
#include <functional>

namespace dqueue {

using rawData = std::vector<uint8_t>;
using DataHandler =
    std::function<void(const std::string &queueName, const rawData &d, Id id)>;

class IQueueClient {
public:
  virtual ~IQueueClient() {}
  virtual void addHandler(DataHandler handler) = 0;
  virtual void createQueue(const QueueSettings &settings) = 0;
  virtual void subscribe(const std::string &qname) = 0;
  virtual void unsubscribe(const std::string &qname) = 0;
  virtual void publish(const std::string &qname, const rawData &data) = 0;
};
} // namespace dqueue