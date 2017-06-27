#pragma once
#include <string>

namespace dqueue {
enum class SubscribeActions : uint8_t { Create, Subscribe, Unsubscribe };
struct SubscriptionSettings {
  SubscribeActions action;
  std::string queueName;
  SubscriptionSettings() = default;
  SubscriptionSettings(SubscribeActions a, const std::string &queueName_)
      : action(a), queueName(queueName_) {}
};

struct PublishParams {
  std::string queueName;

  PublishParams() = default;
  PublishParams(const std::string &qName) : queueName(qName) {}
};
} // namespace dqueue