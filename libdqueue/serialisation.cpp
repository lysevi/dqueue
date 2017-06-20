#include <libdqueue/serialisation.h>

namespace dqueue {
namespace serialisation {

template <> size_t get_size_of<std::string>(const std::string &s) {
  return sizeof(uint32_t) + s.length();
}

template <>
void write_value<std::string>(std::vector<uint8_t> &buffer, size_t &offset,
                              const std::string &s) {
  auto len = static_cast<uint32_t>(s.size());
  std::memcpy(buffer.data() + offset, &len, sizeof(uint32_t));
  std::memcpy(buffer.data() + offset + sizeof(uint32_t), s.data(), s.size());
}

} // namespace serialisation
} // namespace dqueue
