#include <libdqueue/node.h>
#include <map>
#include <set>
#include <shared_mutex>

using namespace dqueue;

struct Node::Private {
  Private(const Settings &settigns, dataHandler dh) : _settigns(settigns) {
    _handler = dh;
    nextQueueId = 0;
  }

  void createQueue(const QueueSettings &qsettings) {
    ENSURE(!qsettings.name.empty());

    std::lock_guard<std::shared_mutex> lg(_queues_locker);

    Queue new_q(qsettings);
    new_q.queueId = nextQueueId++;
    _queues.emplace(std::make_pair(new_q.settings.name, new_q));
  }

  std::vector<QueueDescription> getDescription() const {
    std::shared_lock<std::shared_mutex> sl(_queues_locker);
    std::vector<QueueDescription> result(_queues.size());
    size_t pos = 0;
    for (auto kv : _queues) {
      QueueDescription qd{kv.second.settings};
      {
        std::shared_lock<std::shared_mutex> subscr_sm(_subscriptions_locker);
        auto it = _subscriptions.find(kv.second.queueId);
        if (it != _subscriptions.end()) {
          qd.subscribers.reserve(it->second.size());
          for (const auto &s : it->second) {
            qd.subscribers.push_back(s);
          }
        }
      }
      result[pos++] = qd;
    }
    return result;
  }

  void addClient(const Client &c) {
    std::lock_guard<std::shared_mutex> lg(_clients_locker);
    _clients[c.id] = c;
  }

  std::vector<ClientDescription> getClientsDescription() const {
    std::shared_lock<std::shared_mutex> sm(_clients_locker);
    std::vector<ClientDescription> result(_clients.size());
    size_t pos = 0;
    for (auto kv : _clients) {
      ClientDescription cd{kv.second.id};
      {
        std::shared_lock<std::shared_mutex> subscr_sm(_subscriptions_locker);
        for (const auto &subscr : _subscriptions) {
          if (subscr.second.find(kv.second.id) != subscr.second.end()) {
            cd.subscribtions.push_back(queueById(subscr.first).settings.name);
          }
        }
      }
      result[pos++] = cd;
    }
    return result;
  }

  Queue queueById(int id) const {
    std::shared_lock<std::shared_mutex> sl(_queues_locker);
    for (auto kv : _queues) {
      if (kv.second.queueId == id) {
        return kv.second;
      }
    }
    THROW_EXCEPTION("queue with id=", id, " was not founded.");
  }

  Queue queueByName(const std::string &name) const {
    std::shared_lock<std::shared_mutex> sl(_queues_locker);
    auto it = _queues.find(name);
    if (it == _queues.end()) {
      THROW_EXCEPTION("queue with name=", name, " was not founded.");
    }
    return it->second;
  }

  void changeSubscription(SubscribeActions action, std::string queueName, int clientId) {
    int qId = 0;

    auto q = queueByName(queueName);
    qId = q.queueId;

    {
      std::shared_lock<std::shared_mutex> sm(_clients_locker);
      auto it = _clients.find(clientId);
      if (it == _clients.end()) {
        THROW_EXCEPTION("node: clientId=", clientId, " does not exists");
      }
    }

    std::lock_guard<std::shared_mutex> sm(_subscriptions_locker);
    switch (action) {
    case dqueue::Node::SubscribeActions::Subscribe: {
      _subscriptions[qId].insert(clientId);
      break;
    }
    case dqueue::Node::SubscribeActions::Unsubscribe: {
      _subscriptions[qId].erase(clientId);
      break;
    }
    default:
      THROW_EXCEPTION("node: unknow action:", (int)action);
      break;
    }
  }

  void publish(const std::string &qname, const Node::rawData &rd) {
    int qId = queueByName(qname).queueId;
    std::set<int> local_cpy;

    {
      std::shared_lock<std::shared_mutex> sl(_subscriptions_locker);
      local_cpy = _subscriptions[qId];
    }

    for (auto clientId : local_cpy) {
      _handler(rd, clientId);
    }
  }

  Settings _settigns;

  mutable std::shared_mutex _queues_locker;
  std::map<std::string, Queue> _queues; // name to queue
  int nextQueueId;

  mutable std::shared_mutex _clients_locker;
  std::map<int, Client> _clients; // id to client

  mutable std::shared_mutex _subscriptions_locker;
  std::map<int, std::set<int>> _subscriptions; // qId 2 userId

  Node::dataHandler _handler;
};

Node::Node(const Settings &settigns, dataHandler dh)
    : _impl(std::make_unique<Node::Private>(settigns, dh)) {}

Node::~Node() {
  _impl = nullptr;
}

void Node::createQueue(const QueueSettings &qsettings) {
  _impl->createQueue(qsettings);
}

std::vector<Node::QueueDescription> Node::getQueuesDescription() const {
  return _impl->getDescription();
}

void Node::addClient(const Client &c) {
  _impl->addClient(c);
}

std::vector<Node::ClientDescription> Node::getClientsDescription() const {
  return _impl->getClientsDescription();
}

void Node::changeSubscription(Node::SubscribeActions action, std::string queueName,
                              int clientId) {
  return _impl->changeSubscription(action, queueName, clientId);
}

void Node::publish(const std::string &qname, const Node::rawData &rd) {
  return _impl->publish(qname, rd);
}