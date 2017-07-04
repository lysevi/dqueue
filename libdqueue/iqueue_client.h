#pragma once

#include <libdqueue/exports.h>
#include <libdqueue/node_settings.h>
#include <libdqueue/q.h>
#include <libdqueue/users.h>
#include <libdqueue/utils/utils.h>
#include <functional>
#include <set>
#include <shared_mutex>
#include <type_traits>
#include <unordered_map>
#include <utility>
namespace dqueue {
using DataHandler =
    std::function<void(const PublishParams &info, const std::vector<uint8_t> &d, Id id)>;

class EventConsumer {
public:
  static const Id DefaultId = std::numeric_limits<Id>::max();
  EventConsumer() { _consumerId = DefaultId; }
  void setId(Id id_) { _consumerId = id_; }
  Id getId() const { return _consumerId; }

  virtual void consume(const PublishParams &info, const std::vector<uint8_t> &d, Id id) = 0;

private:
  Id _consumerId;
};

class LambdaEventConsumer : public EventConsumer {
public:
  LambdaEventConsumer() = default;
  LambdaEventConsumer(DataHandler dh) { _handler = dh; }
  void consume(const PublishParams &info, const std::vector<uint8_t> &d, Id id_) override {
    _handler(info, d, id_);
  }

protected:
  DataHandler _handler;
};

enum class OperationType { Async, Sync };

class IQueueClient {
public:
  virtual ~IQueueClient() {}

  void rmHandler(EventConsumer *handler) {
    std::lock_guard<std::shared_mutex> lg(_eventHandlers_locker);
    _eventHandlers.erase(handler->getId());
  }

  void callConsumer(const PublishParams &info, const std::vector<uint8_t> &d, Id id) {
    std::shared_lock<std::shared_mutex> lg(_eventHandlers_locker);
    auto it = _queue2handler.find(info.queueName);
    if (it != _queue2handler.end()) {
      for (auto v : it->second) {
        auto es = _eventHandlers[v];
        if (es.second.checkTag(info.tag)) {
          es.first->consume(info, d, id);
        }
      }
    }
  }

  virtual void createQueue(const QueueSettings &settings,
                           const OperationType ot = OperationType::Sync) = 0;
  virtual void subscribe(const SubscriptionParams &settings, EventConsumer *handler,
                         const OperationType ot = OperationType::Sync) = 0;
  virtual void unsubscribe(const std::string &qname,
                           const OperationType ot = OperationType::Sync) = 0;
  virtual void publish(const PublishParams &settings, const std::vector<uint8_t> &data,
                       const OperationType ot = OperationType::Sync) = 0;

protected:
  void addHandler(const SubscriptionParams &settings, EventConsumer *handler) {
    if (handler != nullptr) {
      std::lock_guard<std::shared_mutex> lg(_eventHandlers_locker);
      if (handler->getId() != EventConsumer::DefaultId) {
        auto it = _eventHandlers.find(handler->getId());
        if (it == _eventHandlers.end()) {
          THROW_EXCEPTION("Reuse  of the handler");
        }
        it->second.second = settings;
        _queue2handler[settings.queueName].insert(handler->getId());
      } else {
        handler->setId(_nextConsumersId++);
        _eventHandlers[handler->getId()] = std::make_pair(handler, settings);
        _queue2handler[settings.queueName].insert(handler->getId());
      }
    }
  }

private:
  std::shared_mutex _eventHandlers_locker;
  Id _nextConsumersId = 0;
  std::unordered_map<Id, std::pair<EventConsumer *, SubscriptionParams>> _eventHandlers;
  std::unordered_map<std::string, std::set<Id>> _queue2handler;
};
} // namespace dqueue