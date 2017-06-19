#pragma once

#include <libdqueue/utils/utils.h>
#include <libdqueue/network_message.h>
#include <memory>
#include <stdint.h>
#include <string>
namespace dqueue {
struct QueueSettings {
  std::string name;
  QueueSettings() { }
  QueueSettings(const std::string &name_) { name = name_; }

  EXPORT NetworkMessage_ptr toNetworkMessage()const;
  EXPORT static QueueSettings fromNetworkMessage(const NetworkMessage_ptr&nd);
};

struct Queue {
  QueueSettings settings;
  uint64_t last_update;
  Queue(const QueueSettings &settings_) : settings(settings_) { updateTime(); }
  void updateTime();
};
} // namespace dqueue