#include <libdqueue/network_message.h>
#include <cstring>

using namespace dqueue;

network_message::network_message() {
  memset(data, 0, MAX_MESSAGE_SIZE);
  size = 0;
}

network_message::network_message(const message_kind &kind_) {
  memset(data, 0, MAX_MESSAGE_SIZE);
  size = sizeof(kind_);
  auto ptr_to_kind = reinterpret_cast<message_kind *>(this->data);
  *ptr_to_kind = kind_;
}

network_message::~network_message() {}

std::tuple<network_message::message_size, uint8_t *> network_message::as_buffer() {
  uint8_t *v = reinterpret_cast<uint8_t *>(this);
  auto buf_size = static_cast<message_size>(SIZE_OF_MESSAGE_SIZE + size);
  return std::tie(buf_size, v);
}
