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
  Private(const Settings &settigns, DataHandler dh, const UserBase_Ptr&ub) : _settigns(settigns) {
    _handler = dh;
    nextQueueId = 0;
	_clients = ub;
  }

  void createQueue(const QueueSettings &qsettings, const Id ownerId) {
    ENSURE(!qsettings.name.empty());
    logger_info("node: create queue:", qsettings.name, ", owner: #", ownerId);

	if (!_clients->exists(ownerId)) {
		THROW_EXCEPTION("node: clientId=", ownerId, " does not exists");
	}

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


  void eraseClient(const Id id) {
    logger_info("node: erase client #", id);
    std::lock(_subscriptions_locker, _queues_locker);
    _clients->erase(id);

    // erase _subscriptions
    for (auto &s : _subscriptions) {
      auto it = s.second.find(id);
      if (it != s.second.end()) {
        s.second.erase(id);

        // erase empty queue.
        bool must_be_removed = s.second.empty();
        if (!must_be_removed) {
          auto subscr_it = s.second.find(ServerID);
          if (subscr_it != s.second.end()) {
            // if only server subscription, and server not a owner of a queue.
            must_be_removed = !subscr_it->second.isowner;
          }
        }

        if (must_be_removed) {
          auto qname = s.first;
          logger_info("server: erase queue ", qname);

          _queues.erase(qname);
        }
      }
    }
    _subscriptions_locker.unlock();
    _queues_locker.unlock();
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

  void changeSubscription(SubscribeActions action, const std::string &queueName,
	  Id clientId) {
    logger_info("node: changeSubscription:", queueName, ",  #", clientId);
    queueByName(queueName);

    {
      if (!_clients->exists(clientId)) {
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

  void publish(const std::string &qname, const rawData &rd) {
    queueByName(qname);
    std::map<Id, QueueListener> local_cpy;

    {
      std::shared_lock<std::shared_mutex> sl(_subscriptions_locker);
      local_cpy = _subscriptions[qname];
    }

    for (auto clientId : local_cpy) {
      /* if (clientId.second.isowner && !clientId.second.issubscribed) {
         continue;
       }*/
      _handler(qname, rd, clientId.first);
    }
  }

  Settings _settigns;

  UserBase_Ptr _clients;

  // TODO all dict is name2value. not id2value.
  mutable std::shared_mutex _queues_locker;
  std::map<std::string, Queue> _queues; // name to queue
  int nextQueueId;

  mutable std::shared_mutex _subscriptions_locker;
  std::map<std::string, std::map<Id, QueueListener>>
      _subscriptions; // queue 2 (userId listener)

  DataHandler _handler;
};

Node::Node(const Settings &settigns, DataHandler dh, const UserBase_Ptr&ub)
    : _impl(std::make_unique<Node::Private>(settigns, dh, ub)) {}

Node::~Node() {
  _impl = nullptr;
}

void Node::createQueue(const QueueSettings &qsettings, const Id ownerId) {
  _impl->createQueue(qsettings, ownerId);
}

std::vector<Node::QueueDescription> Node::getQueuesDescription() const {
  return _impl->getDescription();
}

void Node::eraseClient(const Id id) {
  _impl->eraseClient(id);
}


void Node::changeSubscription(Node::SubscribeActions action, const std::string &queueName,
	Id clientId) {
  return _impl->changeSubscription(action, queueName, clientId);
}

void Node::publish(const std::string &qname, const rawData &rd) {
  return _impl->publish(qname, rd);
}