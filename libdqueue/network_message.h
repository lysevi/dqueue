#pragma once

#include <libdqueue/exports.h>
#include <libdqueue/utils/async/locker.h>
#include <cstring>

namespace dqueue {
#pragma pack(push, 1)

struct NetworkMessage {
  using message_size = uint32_t;
  using message_kind = uint16_t;

  struct message_header {
    message_kind kind;
  };

  static const size_t MAX_MESSAGE_SIZE = 1024*1024*4;
  static const size_t SIZE_OF_MESSAGE_SIZE = sizeof(message_size);
  static const size_t MAX_BUFFER_SIZE = MAX_MESSAGE_SIZE - sizeof(message_header);

  message_size size;
  uint8_t data[MAX_MESSAGE_SIZE];

  NetworkMessage() {
    memset(data, 0, MAX_MESSAGE_SIZE);
    size = 0;
  }

  NetworkMessage(const message_kind &kind_) {
    memset(data, 0, MAX_MESSAGE_SIZE);
    size = sizeof(kind_);
    cast_to_header()->kind = kind_;
  }

  ~NetworkMessage() {}

  std::tuple<message_size, uint8_t *> as_buffer() {
    uint8_t *v = reinterpret_cast<uint8_t *>(this);
    auto buf_size = static_cast<message_size>(SIZE_OF_MESSAGE_SIZE + size);
    return std::tie(buf_size, v);
  }

  message_header *cast_to_header() {
    return reinterpret_cast<message_header *>(this->data);
  }
};

#pragma pack(pop)

using NetworkMessage_ptr = std::shared_ptr<NetworkMessage>;
}
