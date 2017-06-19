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

  static const size_t MAX_MESSAGE_SIZE = 1024 * 1024 * 4;
  static const size_t SIZE_OF_MESSAGE_SIZE = sizeof(message_size);
  static const size_t MAX_BUFFER_SIZE = MAX_MESSAGE_SIZE - sizeof(message_header);

  message_size *size;
  uint8_t data[MAX_MESSAGE_SIZE];

  NetworkMessage(size_t sz) {
    memset(data, 0, MAX_MESSAGE_SIZE);
    size = (message_size *)data;
    *size = static_cast<message_size>(sz + SIZE_OF_MESSAGE_SIZE);
  }

  NetworkMessage(size_t sz, const message_kind &kind_) : NetworkMessage(sz) {
    *size += sizeof(message_kind);
    cast_to_header()->kind = kind_;
  }

  ~NetworkMessage() {}

  uint8_t *value() { return (data + sizeof(message_size) + sizeof(message_kind)); }

  std::tuple<message_size, uint8_t *> as_buffer() {
    uint8_t *v = reinterpret_cast<uint8_t *>(data);
    auto buf_size = *size;
    return std::tie(buf_size, v);
  }

  message_header *cast_to_header() {
    return reinterpret_cast<message_header *>(this->data + SIZE_OF_MESSAGE_SIZE);
  }
};

#pragma pack(pop)

using NetworkMessage_ptr = std::shared_ptr<NetworkMessage>;
}
