#include <libdqueue/node.h>
#include <map>
#include <shared_mutex>

using namespace dqueue;

struct Node::Private {
  Private(const Settings &settigns) : _settigns(settigns) {}
  void createQueue(const QueueSettings &qsettings) {
    ENSURE(!qsettings.name.empty());

    std::lock_guard<std::shared_mutex> lg(_queues_locker);

    Queue new_q(qsettings);
    _queues.emplace(std::make_pair(qsettings.name, new_q));
  }

  std::vector<QueueDescription> getDescription()const {
    std::shared_lock<std::shared_mutex> sl(_queues_locker);
    std::vector<QueueDescription> result(_queues.size());
    size_t pos = 0;
    for (auto kv : _queues) {
      QueueDescription qd{kv.second.settings};
      result[pos++] = qd;
    }
    return result;
  }

  Settings _settigns;

  mutable std::shared_mutex _queues_locker;
  std::map<std::string, Queue> _queues;
};

Node::Node(const Settings &settigns) : _impl(std::make_unique<Node::Private>(settigns)) {}

Node::~Node() {
  _impl = nullptr;
}

void Node::createQueue(const QueueSettings &qsettings) {
  _impl->createQueue(qsettings);
}

std::vector<Node::QueueDescription> Node::getDescription() const {
  return _impl->getDescription();
}