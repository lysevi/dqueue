#pragma once

#include <libdqueue/network_message.h>
#include <type_traits>

namespace dqueue {
enum class MessageKinds : NetworkMessage::message_kind {
  OK,
  LOGIN,
  CREATE_QUEUE,
  SUBSCRIBE,
  UNSUBSCRIBE,
  PUBLISH,
};

const uint64_t LoginConfirmedID = std::numeric_limits<uint64_t>::max();
} // namespace dqueue