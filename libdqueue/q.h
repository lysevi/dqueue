#pragma once

#include <libdqueue/network_message.h>
#include <libdqueue/utils/utils.h>
#include <memory>
#include <stdint.h>
#include <string>
namespace dqueue {

struct QueueSettings {
  std::string name;
  QueueSettings() {}
  QueueSettings(const std::string &name_) { name = name_; }
};

struct Queue {
  int queueId;
  QueueSettings settings;
  Queue(const QueueSettings &settings_) : settings(settings_) {}
};
} // namespace dqueue
