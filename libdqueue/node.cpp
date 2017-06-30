#include <libdqueue/node.h>
#include <map>
#include <set>
#include <shared_mutex>

using namespace dqueue;

namespace {
struct QueueListener {
  bool isowner = false;      // is creator of queue
  bool issubscribed = false; // if true, owner is subscribed.
  SubscriptionParams settings;
};

} // namespace

struct Node::Private {
  Private(const Settings &settigns, DataHandler dh, const UserBase_Ptr &ub)
      : _settigns(settigns) {
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
    SubscriptionParams ss(qsettings.name);
    changeSubscription(SubscribeActions::Create, ss, ownerId);
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
    logger_info("node: start erasing client id=#", id);
    std::lock(_subscriptions_locker, _queues_locker);
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
    THROW_EXCEPTION("queue with id=#", id, " was not founded.");
  }

  Queue queueByName(const std::string &name) const {
    std::shared_lock<std::shared_mutex> sl(_queues_locker);
    auto it = _queues.find(name);
    if (it == _queues.end()) {
      THROW_EXCEPTION("queue with name=", name, " was not founded.");
    }
    return it->second;
  }

  bool queueIsExists(const std::string &name) const {
    std::shared_lock<std::shared_mutex> sl(_queues_locker);
    auto it = _queues.find(name);
    return (it != _queues.end());
  }

  void changeSubscription(SubscribeActions action, const SubscriptionParams &settings,
                          Id clientId) {
    logger_info("node: changeSubscription:", settings.queueName, ",  id=#", clientId);

    if (!queueIsExists(settings.queueName)) { // check queue is exists.
      switch (action) {
      case SubscribeActions::Create:
        break;
      case SubscribeActions::Subscribe:
        logger_fatal("node: subscription to not exists queue: ", settings.queueName,
                     ",  id=#", clientId);
        break;
      case SubscribeActions::Unsubscribe:
        return;
      }
    }

    {
      if (!_clients->exists(clientId)) {
        THROW_EXCEPTION("node: clientId=#", clientId, " does not exists");
      }
    }

    std::lock_guard<std::shared_mutex> sm(_subscriptions_locker);
    bool isOwner = action == SubscribeActions::Create;

    switch (action) {
    case SubscribeActions::Create:
    case SubscribeActions::Subscribe: {
      auto clients = &_subscriptions[settings.queueName];
      QueueListener ql;
      ql.isowner = isOwner;
      ql.settings = settings;
      auto it = clients->find(clientId);
      if (it != clients->end()) {
        if (!isOwner) {
          it->second.issubscribed = true;
          it->second.settings = settings;
        }
      } else {
        _subscriptions[settings.queueName][clientId] = ql;
      }

      break;
    }
    case SubscribeActions::Unsubscribe: {
      _subscriptions[settings.queueName].erase(clientId);
      break;
    }
      /*default:
        THROW_EXCEPTION("node: unknow action:", (int)action);*/
    }
  }

  void publish(const PublishParams &pubParam, const rawData &rd, Id author) {
    User autorDescr;
    if (!_clients->byId(author, autorDescr)) {
      logger_fatal("server: #", author, " user does not exists.");
    }
    if (!queueIsExists(pubParam.queueName)) {
      logger("server: client", autorDescr.login,
             " publish to not exists queue:", pubParam.queueName);
      return;
    }
    std::map<Id, QueueListener> local_cpy;

    {
      std::shared_lock<std::shared_mutex> sl(_subscriptions_locker);
      local_cpy = _subscriptions[pubParam.queueName];
    }

    for (auto clientId : local_cpy) {
      // owner receive messages only if subscribe to queue
      if (!clientId.second.isowner || clientId.second.issubscribed) {
        /* if (clientId.second.isowner && !clientId.second.issubscribed) {
           continue;
         }*/
        bool isTarget = false;
        if (clientId.second.settings.tag.empty()) {
          isTarget = true;
        } else {
          isTarget = clientId.second.settings.checkTag(pubParam.tag);
        }
        if (isTarget) {
          _handler(pubParam, rd, clientId.first);
        }
      }
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

Node::Node(const Settings &settigns, DataHandler dh, const UserBase_Ptr &ub)
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

void Node::changeSubscription(SubscribeActions action, const SubscriptionParams &settings,
                              Id clientId) {
  return _impl->changeSubscription(action, settings, clientId);
}

void Node::publish(const PublishParams &settings, const rawData &rd, Id author) {
  return _impl->publish(settings, rd, author);
}