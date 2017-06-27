#pragma once
#include <regex>
#include <string>

namespace dqueue {
enum class SubscribeActions : uint8_t { Create, Subscribe, Unsubscribe };
struct SubscriptionParams {
  std::string queueName;
  std::string tag;
  SubscriptionParams() = default;
  SubscriptionParams(const std::string &queueName_) : queueName(queueName_) {}
  SubscriptionParams(const std::string &queueName_, const std::string &tag_)
      : queueName(queueName_), tag(tag_) {}

  bool checkTag(const std::string &tag_) {
    if (tag.empty()) {
      return true;
    } else {
      // TODO compile and save to cache!
      std::regex tag_regex(tag);
      return std::regex_match(tag_, tag_regex);
    }
  }
};

struct PublishParams {
  std::string queueName;
  std::string tag;
  PublishParams() = default;
  PublishParams(const std::string &qName) : queueName(qName) {}
  PublishParams(const std::string &qName, const std::string &tag_)
      : queueName(qName), tag(tag_) {}
};
} // namespace dqueue