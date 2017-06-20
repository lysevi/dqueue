#pragma once

#include <libdqueue/exports.h>
#include <cstddef>
#include <cstring>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace dqueue {
namespace serialisation {

template <class T> size_t get_size_of(const T &) {
  return sizeof(T);
}

template <> EXPORT size_t get_size_of<std::string>(const std::string &s);

template <typename S>
void write_value(std::vector<uint8_t> &buffer, size_t &offset, const S &s) {
  std::memcpy(buffer.data() + offset, &s, sizeof(s));
}

template <typename S>
void read_value(std::vector<uint8_t> &buffer, size_t &offset, S &s) {
  std::memcpy(&s, buffer.data() + offset, sizeof(s));
}

template <>
EXPORT void read_value<std::string>(std::vector<uint8_t> &buffer, size_t &offset,
                                    std::string &s);

template <>
void EXPORT write_value<std::string>(std::vector<uint8_t> &buffer, size_t &offset,
                                     const std::string &s);

template <typename... T> struct Scheme {
  std::vector<uint8_t> buffer;
  size_t offset;

  template <typename Head> static void calculate_size_rec(size_t &result, Head &&head) {
    result += get_size_of(head);
  }

  template <typename Head, typename... Tail>
  static void calculate_size_rec(size_t &result, Head &&head, Tail &&... t) {
    result += get_size_of(std::forward<Head>(head));
    calculate_size_rec(result, std::forward<Tail>(t)...);
  }

  static size_t size_of_args(T &&... args) {
    size_t result = 0;
    calculate_size_rec(result, std::forward<T>(args)...);
    return result;
  }

  template <typename Head>
  static void write_args(std::vector<uint8_t> &buffer, size_t &offset, Head &&head) {
    auto szofcur = get_size_of(head);
    write_value(buffer, offset, head);
    offset += szofcur;
  }

  template <typename Head, typename... Tail>
  static void write_args(std::vector<uint8_t> &buffer, size_t &offset, Head &&head,
                         Tail &&... t) {
    auto szofcur = get_size_of(head);
    write_value(buffer, offset, head);
    offset += szofcur;
    write_args(buffer, offset, std::forward<Tail>(t)...);
  }

  template <typename Head>
  static void read_args(std::vector<uint8_t> &buffer, size_t &offset, Head &&head) {
    auto szofcur = get_size_of(head);
    read_value(buffer, offset, head);
    offset += szofcur;
  }

  template <typename Head, typename... Tail>
  static void read_args(std::vector<uint8_t> &buffer, size_t &offset, Head &&head,
                        Tail &&... t) {
    auto szofcur = get_size_of(head);
    read_value(buffer, offset, head);
    offset += szofcur;
    read_args(buffer, offset, std::forward<Tail>(t)...);
  }

  Scheme() { offset = size_t(0); }

  Scheme(T &&... t) {
    offset = 0;
    auto sz = size_of_args(std::forward<T>(t)...);
    buffer.resize(sz);

    write_args(buffer, offset, std::forward<T>(t)...);
  }

  void init_from_buffer(const std::vector<uint8_t> &buf) { buffer = buf; }

  void readTo(T &... t) {
    offset = 0;
    read_args(buffer, offset, std::forward<T>(t)...);
  }
};

} // namespace serialisation
} // namespace dqueue