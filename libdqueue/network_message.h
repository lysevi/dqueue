#pragma once

#include <libdqueue/exports.h>
#include <libdqueue/utils/async/locker.h>

namespace dqueue {
#pragma pack(push, 1)

struct NetworkMessage {
  using message_size = uint16_t;
  using message_kind = uint16_t;

  struct message_header {
	  NetworkMessage::message_kind kind;
  };

  static const size_t MAX_MESSAGE_SIZE = std::numeric_limits<message_size>::max();
  static const size_t SIZE_OF_MESSAGE_SIZE = sizeof(NetworkMessage::message_size);
  static const size_t MAX_BUFFER_SIZE = MAX_MESSAGE_SIZE - sizeof(message_header);
  
  message_size size;
  uint8_t data[MAX_MESSAGE_SIZE];

  NetworkMessage::NetworkMessage() {
	  memset(data, 0, MAX_MESSAGE_SIZE);
	  size = 0;
  }

  NetworkMessage::NetworkMessage(const message_kind &kind_) {
	  memset(data, 0, MAX_MESSAGE_SIZE);
	  size = sizeof(kind_);
	  cast_to_header()->kind = kind_;
  }

  NetworkMessage::~NetworkMessage() {}

  std::tuple<NetworkMessage::message_size, uint8_t *> NetworkMessage::as_buffer() {
	  uint8_t *v = reinterpret_cast<uint8_t *>(this);
	  auto buf_size = static_cast<message_size>(SIZE_OF_MESSAGE_SIZE + size);
	  return std::tie(buf_size, v);
  }

  message_header* cast_to_header() {
	  return reinterpret_cast<message_header*>(this->data);
  }
};

#pragma pack(pop)

using NetworkMessage_ptr = std::shared_ptr<NetworkMessage>;


}
