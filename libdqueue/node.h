#pragma once

#include <libdqueue/exports.h>
#include <libdqueue/q.h>
#include <functional>

namespace dqueue {
class Node {
public:
  enum class SubscribeActions : uint8_t { Subscribe, Unsubscribe };

  struct Settings {};
  struct QueueDescription {
    QueueSettings settings;
    std::vector<int> subscribers;
  };

  struct Client {
    int id;
  };

  struct ClientDescription {
    int id;
    std::vector<std::string> subscribtions;
  };
  using rawData = std::vector<uint8_t>;
  using dataHandler = std::function<void(const rawData &d, int id)>;

  EXPORT Node(const Settings &settigns, dataHandler dh);
  EXPORT ~Node();
  EXPORT void createQueue(const QueueSettings &qsettings);
  EXPORT std::vector<QueueDescription> getQueuesDescription() const;

  EXPORT void addClient(const Client &c);
  EXPORT std::vector<ClientDescription> getClientsDescription() const;

  EXPORT void changeSubscription(SubscribeActions action, std::string queueName,
                                 int clientId);

  EXPORT void publish(const std::string &qname, const rawData &rd);

protected:
  struct Private;
  std::unique_ptr<Private> _impl;
};
} // namespace dqueue