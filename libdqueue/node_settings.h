#pragma once
#include <string>

namespace dqueue {
enum class SubscribeActions : uint8_t { Create, Subscribe, Unsubscribe };
struct SubscriptionParams {
  std::string queueName;
  SubscriptionParams() = default;
  SubscriptionParams(const std::string &queueName_)
      : queueName(queueName_) {}
};

struct PublishParams {
  std::string queueName;

  PublishParams() = default;
  PublishParams(const std::string &qName) : queueName(qName) {}
};
} // namespace dqueue