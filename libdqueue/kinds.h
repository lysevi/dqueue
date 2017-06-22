#pragma once

#include <libdqueue/network_message.h>

namespace dqueue {
enum class MessageKinds : NetworkMessage::message_kind {
  OK,
  CREATE_QUEUE,
  SUBSCRIBE,
  UNSUBSCRIBE,
  PUBLISH,
};
}