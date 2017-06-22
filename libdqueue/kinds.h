#pragma once

#include <libdqueue/network_message.h>
#include <type_traits>

namespace dqueue {
enum class MessageKinds : NetworkMessage::message_kind {
  OK,
  LOGIN,
  LOGIN_CONFIRM,
  CREATE_QUEUE,
  SUBSCRIBE,
  UNSUBSCRIBE,
  PUBLISH,
};
} // namespace dqueue