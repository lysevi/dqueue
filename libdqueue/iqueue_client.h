#pragma once

#include <libdqueue/exports.h>
#include <libdqueue/q.h>
#include <libdqueue/users.h>
#include <libdqueue/utils/utils.h>
#include <functional>
#include <set>
#include <shared_mutex>
#include <unordered_map>

namespace dqueue {

using rawData = std::vector<uint8_t>;
// TODO Id must be std::vector<Id> for speedup and less memory allocations.
using DataHandler =
    std::function<void(const std::string &queueName, const rawData &d, Id id)>;

class EventConsumer {
public:
  void setId(int id_) { _consumerId = id_; }
  int getId() const { return _consumerId; }

  virtual void consume(const std::string &queueName, const rawData &d, Id id) = 0;

private:
  int _consumerId;
};

class LambdaEventConsumer : public EventConsumer {
public:
  LambdaEventConsumer(DataHandler dh) { _handler = dh; }
  void consume(const std::string &queueName, const rawData &d, Id id_) override {
    _handler(queueName, d, id_);
  }

protected:
  DataHandler _handler;
};

class IQueueClient {
public:
  virtual ~IQueueClient() {}
  void rmHandler(EventConsumer *handler) {
    std::lock_guard<std::shared_mutex> lg(_eventHandlers_locker);
    _eventHandlers.erase(handler->getId());
  }

  void callConsumer(const std::string &queueName, const rawData &d, Id id) {
    std::shared_lock<std::shared_mutex> lg(_eventHandlers_locker);
    auto it = _queue2handler.find(queueName);
    if (it != _queue2handler.end()) {
      for (auto v : it->second) {
        _eventHandlers[v]->consume(queueName, d, id);
      }
    }
  }

  virtual void createQueue(const QueueSettings &settings) = 0;
  virtual void subscribe(const std::string &qname, EventConsumer *handler) = 0;
  virtual void unsubscribe(const std::string &qname) = 0;
  virtual void publish(const std::string &qname, const rawData &data) = 0;

protected:
  void addHandler(const std::string &queueName, EventConsumer *handler) {
    if (handler != nullptr) {
      std::lock_guard<std::shared_mutex> lg(_eventHandlers_locker);
      handler->setId(_nextConsumersId++);
      _eventHandlers[handler->getId()] = handler;
      _queue2handler[queueName].insert(handler->getId());
    }
  }

private:
  std::shared_mutex _eventHandlers_locker;
  int _nextConsumersId = 0;
  std::unordered_map<int, EventConsumer *> _eventHandlers;
  std::unordered_map<std::string, std::set<int>> _queue2handler;
};
} // namespace dqueue