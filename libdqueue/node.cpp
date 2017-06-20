#include <libdqueue/node.h>
#include <map>
#include <set>
#include <shared_mutex>

using namespace dqueue;

namespace {
struct QueueListener {
  bool isowner = false;      // is creator of queue
  bool issubscribed = false; // if true, owner is subscribed.
};

} // namespace

struct Node::Private {
  Private(const Settings &settigns, dataHandler dh) : _settigns(settigns) {
    _handler = dh;
    nextQueueId = 0;
  }

  void createQueue(const QueueSettings &qsettings, const int ownerId) {
    ENSURE(!qsettings.name.empty());

    clientById(ownerId);
    {
      std::lock_guard<std::shared_mutex> lg(_queues_locker);

      Queue new_q(qsettings);
      new_q.queueId = nextQueueId++;
      _queues.emplace(std::make_pair(new_q.settings.name, new_q));
    }
    changeSubscription(Node::SubscribeActions::Create, qsettings.name, ownerId);
  }

  std::vector<QueueDescription> getDescription() const {
    std::shared_lock<std::shared_mutex> sl(_queues_locker);
    std::vector<QueueDescription> result(_queues.size());
    size_t pos = 0;
    for (auto kv : _queues) {
      QueueDescription qd(kv.second.settings);
      {
        std::shared_lock<std::shared_mutex> subscr_sm(_subscriptions_locker);
        auto it = _subscriptions.find(kv.first);
        if (it != _subscriptions.end()) {
          auto values = it->second;
          subscr_sm.unlock();

          qd.subscribers.reserve(values.size());
          for (const auto &s : values) {
            qd.subscribers.push_back(s.first);
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

  void eraseClient(const int id) {
    std::lock(_clients_locker, _subscriptions_locker, _queues_locker);
    _clients.erase(id);

    for (auto &s : _subscriptions) {
      auto it = s.second.find(id);
      if (it != s.second.end()) {
        s.second.erase(id);

        if (s.second.empty()) {
          auto qname = s.first;
          logger_info("server: erase queue ", qname);

          _queues.erase(qname);
        }
      }
    }
    _clients_locker.unlock();
    _subscriptions_locker.unlock();
    _queues_locker.unlock();
  }

  std::vector<ClientDescription> getClientsDescription() const {
    std::shared_lock<std::shared_mutex> sm(_clients_locker);
    std::vector<ClientDescription> result(_clients.size());
    size_t pos = 0;
    for (auto kv : _clients) {
      ClientDescription cd(kv.second.id);
      {
        std::shared_lock<std::shared_mutex> subscr_sm(_subscriptions_locker);
        for (const auto &subscr : _subscriptions) {
          if (subscr.second.find(kv.second.id) != subscr.second.end()) {
            cd.subscribtions.push_back(queueByName(subscr.first).settings.name);
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

  Client clientById(int id) {
    std::shared_lock<std::shared_mutex> sm(_clients_locker);
    auto it = _clients.find(id);
    if (it == _clients.end()) {
      THROW_EXCEPTION("server: clientById not found #", id);
    }
    return it->second;
  }

  void changeSubscription(SubscribeActions action, std::string queueName, int clientId) {
    queueByName(queueName);

    {
      std::shared_lock<std::shared_mutex> sm(_clients_locker);
      auto it = _clients.find(clientId);
      if (it == _clients.end()) {
        THROW_EXCEPTION("node: clientId=", clientId, " does not exists");
      }
    }

    bool isOwner = action == dqueue::Node::SubscribeActions::Create;

    std::lock_guard<std::shared_mutex> sm(_subscriptions_locker);

    switch (action) {
    case dqueue::Node::SubscribeActions::Create:
    case dqueue::Node::SubscribeActions::Subscribe: {
      auto clients = &_subscriptions[queueName];
      QueueListener ql;
      ql.isowner = isOwner;
      auto it = clients->find(clientId);
      if (it != clients->end()) {
        if (!isOwner) {
          it->second.issubscribed = true;
        }
      } else {
        _subscriptions[queueName][clientId] = ql;
      }
      break;
    }
    case dqueue::Node::SubscribeActions::Unsubscribe: {
      _subscriptions[queueName].erase(clientId);
      break;
    }
    /*default:
      THROW_EXCEPTION("node: unknow action:", (int)action);*/
    }
  }

  void publish(const std::string &qname, const Node::rawData &rd) {
    queueByName(qname);
    std::map<int, QueueListener> local_cpy;

    {
      std::shared_lock<std::shared_mutex> sl(_subscriptions_locker);
      local_cpy = _subscriptions[qname];
    }

    for (auto clientId : local_cpy) {
      /* if (clientId.second.isowner && !clientId.second.issubscribed) {
         continue;
       }*/
      _handler(rd, clientId.first);
    }
  }

  Settings _settigns;

  // TODO all dict is name2value. not id2value.
  mutable std::shared_mutex _queues_locker;
  std::map<std::string, Queue> _queues; // name to queue
  int nextQueueId;

  mutable std::shared_mutex _clients_locker;
  std::map<int, Client> _clients; // id to client

  mutable std::shared_mutex _subscriptions_locker;
  std::map<std::string, std::map<int, QueueListener>>
      _subscriptions; // queue 2 (userId listener)

  Node::dataHandler _handler;
};

Node::Node(const Settings &settigns, dataHandler dh)
    : _impl(std::make_unique<Node::Private>(settigns, dh)) {}

Node::~Node() {
  _impl = nullptr;
}

void Node::createQueue(const QueueSettings &qsettings, const int ownerId) {
  _impl->createQueue(qsettings, ownerId);
}

std::vector<Node::QueueDescription> Node::getQueuesDescription() const {
  return _impl->getDescription();
}

void Node::addClient(const Client &c) {
  _impl->addClient(c);
}

void Node::eraseClient(const int id) {
  _impl->eraseClient(id);
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