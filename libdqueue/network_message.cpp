#include <libdqueue/network_message.h>
#include <cstring>

using namespace dqueue;

NetworkMessage::NetworkMessage() {
  memset(data, 0, MAX_MESSAGE_SIZE);
  size = 0;
}

NetworkMessage::NetworkMessage(const message_kind &kind_) {
  memset(data, 0, MAX_MESSAGE_SIZE);
  size = sizeof(kind_);
  auto ptr_to_kind = reinterpret_cast<message_kind *>(this->data);
  *ptr_to_kind = kind_;
}

NetworkMessage::~NetworkMessage() {}

std::tuple<NetworkMessage::message_size, uint8_t *> NetworkMessage::as_buffer() {
  uint8_t *v = reinterpret_cast<uint8_t *>(this);
  auto buf_size = static_cast<message_size>(SIZE_OF_MESSAGE_SIZE + size);
  return std::tie(buf_size, v);
}
