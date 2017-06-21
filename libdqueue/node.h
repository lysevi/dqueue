#pragma once

#include <libdqueue/exports.h>
#include <libdqueue/q.h>
#include <functional>

namespace dqueue {
class Node {
public:
  static const int ServerID = std::numeric_limits<int>::max();

  enum class SubscribeActions : uint8_t { Create, Subscribe, Unsubscribe };

  struct Settings {};

  struct QueueDescription {
    QueueSettings settings;
    std::vector<int> subscribers;
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

  struct Client {
    int id;
  };

  struct ClientDescription {
    int id;
    std::vector<std::string> subscribtions;

    ClientDescription() : id(std::numeric_limits<int>::max()) {}
    ClientDescription(int id_) : id(id_) {}
  };
  using rawData = std::vector<uint8_t>;
  using dataHandler =
      std::function<void(const std::string &queueName, const rawData &d, int id)>;

  EXPORT Node(const Settings &settigns, dataHandler dh);
  EXPORT ~Node();
  EXPORT void createQueue(const QueueSettings &qsettings, const int ownerId);
  EXPORT std::vector<QueueDescription> getQueuesDescription() const;

  EXPORT void addClient(const Client &c);
  EXPORT void eraseClient(const int id);
  EXPORT std::vector<ClientDescription> getClientsDescription() const;

  EXPORT void changeSubscription(SubscribeActions action, const std::string &queueName,
                                 int clientId);

  EXPORT void publish(const std::string &qname, const rawData &rd);

protected:
  struct Private;
  std::unique_ptr<Private> _impl;
};
} // namespace dqueue