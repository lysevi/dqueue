# dqueue - distributed message queue
[![Build Status](https://travis-ci.org/lysevi/dqueue.svg?branch=master)](https://travis-ci.org/lysevi/dqueue) [![codecov](https://codecov.io/gh/lysevi/dqueue/branch/master/graph/badge.svg)](https://codecov.io/gh/lysevi/dqueue)

# Example
## Server
```C++
#include <libdqueue/dqueue.h>
class SimpleServer final : public virtual dqueue::Server,
                           public dqueue::EventConsumer {
public:
  SimpleServer(boost::asio::io_service *service, dqueue::AbstractServer::Params p)
      : dqueue::Server(service, p) {}

  void queueHandler(const dqueue::PublishParams &info, const std::vector<uint8_t> &d,
                    dqueue::Id) {
    // publish to queue message with tag="server"
    publish(dqueue::PublishParams(info.queueName, "server"), d);
  }

  void onStartComplete() override {
    dqueue::Server::onStartComplete();

    // Create and subscribe to queue messages with tag="client"
    dqueue::QueueSettings qs("serverQ_");
    createQueue(qs);
    dqueue::SubscriptionParams sp(qs.name, "client");
    subscribe(sp, this);
  }
  // called when somebody publish to queue data with tag="client"
  void consume(const dqueue::PublishParams &info, const std::vector<uint8_t> &d,
               dqueue::Id) override {
    this->publish(dqueue::PublishParams(info.queueName, "server"), d);
  }
};
```

## Client
```C++
class SimpleClient final : public dqueue::Client, public dqueue::EventConsumer {
public:
  SimpleClient(boost::asio::io_service *service, const AbstractClient::Params &_params)
      : dqueue::Client(service, _params) {}

  void onConnect() override {
    dqueue::Client::onConnect();
    // subscribe to queue messages with tag="server"
    auto qname = "serverQ_";
    dqueue::SubscriptionParams sp(qname, "server");
    subscribe(sp, this, dqueue::OperationType::Async);
    // publish can by sync or async.
    publish(dqueue::PublishParams(qname, "client"), {1}, dqueue::OperationType::Async);
  }

  void consume(const dqueue::PublishParams &info, const std::vector<uint8_t> &d,
               dqueue::Id) override {
    // publish message with tag="client"
    publish(dqueue::PublishParams(info.queueName, "client"), d, dqueue::OperationType::Async);
    client_sended.fetch_add(1);
  }
};
```
