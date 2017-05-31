#pragma once

#include <libdqueue/exports.h>
#include <libdqueue/utils/async/locker.h>

namespace dqueue {
#pragma pack(push, 1)

struct network_message {
  using message_size = uint16_t;
  using message_kind = uint16_t;

  static const size_t MAX_MESSAGE_SIZE = std::numeric_limits<message_size>::max();

  message_size size;
  uint8_t data[MAX_MESSAGE_SIZE];

  EXPORT network_message();
  EXPORT network_message(const message_kind &kind);
  EXPORT ~network_message();

  EXPORT std::tuple<message_size, uint8_t *> as_buffer();
};

struct message_header {
  network_message::message_kind kind;
};

#pragma pack(pop)

using network_message_ptr = std::shared_ptr<network_message>;

const size_t SIZE_OF_MESSAGE_SIZE = sizeof(network_message::message_size);

}
