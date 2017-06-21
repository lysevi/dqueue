#pragma once

#include <libdqueue/exports.h>
#include <libdqueue/iqueue_client.h>
#include <libdqueue/q.h>
#include <libdqueue/users.h>

namespace dqueue {
class Node {
public:
  enum class SubscribeActions : uint8_t { Create, Subscribe, Unsubscribe };

  struct Settings {};

  struct QueueDescription {
    QueueSettings settings;
    std::vector<Id> subscribers;
    QueueDescription() = default;
    QueueDescription(const QueueDescription &other)
        : settings(other.settings), subscribers(other.subscribers) {}
    QueueDescription(const QueueSettings &settings_) : settings(settings_) {}
    QueueDescription &operator=(const QueueDescription &other) {
      if (this != &other) {
        settings = other.settings;
        subscribers = other.subscribers;
      }
      return *this;
    }
  };


  EXPORT Node(const Settings &settigns, DataHandler dh, const UserBase_Ptr&ub);
  EXPORT ~Node();
  EXPORT void createQueue(const QueueSettings &qsettings, const Id ownerId);
  EXPORT std::vector<QueueDescription> getQueuesDescription() const;

  EXPORT void eraseClient(const Id id);
  
  EXPORT void changeSubscription(SubscribeActions action, const std::string &queueName,
	  Id clientId);

  EXPORT void publish(const std::string &qname, const rawData &rd);

protected:
  struct Private;
  std::unique_ptr<Private> _impl;
};
} // namespace dqueue