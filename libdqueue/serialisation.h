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

template <typename Iterator, typename S> struct writer {
  static void write_value(Iterator it, const S &s) {
    static_assert(std::is_pod<S>::value, "S is not a POD value");
    std::memcpy(&(*it), &s, sizeof(s));
  }
};

template <typename Iterator> struct writer<Iterator, std::string> {
  static void write_value(Iterator it, const std::string &s) {
    auto len = static_cast<uint32_t>(s.size());
    auto ptr = &(*it);
    std::memcpy(ptr, &len, sizeof(uint32_t));
    std::memcpy(ptr + sizeof(uint32_t), s.data(), s.size());
  }
};

template <typename Iterator, typename S> struct reader {
  static void read_value(Iterator &it, S &s) {
    static_assert(std::is_pod<S>::value, "S is not a POD value");
    auto ptr = &(*it);
    std::memcpy(&s, ptr, sizeof(s));
  }
};

template <typename Iterator> struct reader<Iterator, std::string> {
  static void read_value(Iterator &it, std::string &s) {
    uint32_t len = 0;
    auto ptr = &(*it);
    std::memcpy(&len, ptr, sizeof(uint32_t));
    s.resize(len);
    std::memcpy(&s[0], ptr + sizeof(uint32_t), size_t(len));
  }
};

template <class... T> class Scheme {
  template <typename Head> static void calculate_size_rec(size_t &result, const Head &&head) {
    result += get_size_of(head);
  }

  template <typename Head, typename... Tail>
  static void calculate_size_rec(size_t &result, const Head &&head, const Tail &&... t) {
    result += get_size_of(std::forward<const Head>(head));
    calculate_size_rec(result, std::forward<const Tail>(t)...);
  }

  template <class Iterator, typename Head>
  static void write_args(Iterator it, const Head &head) {
    static auto szofcur = get_size_of(head);
    writer<Iterator, Head>::write_value(it, head);
    it += szofcur;
  }

  template <class Iterator, typename Head, typename... Tail>
  static void write_args(Iterator it, const Head &head, const Tail &... t) {
    auto szofcur = get_size_of(head);
    writer<Iterator, Head>::write_value(it, head);
    it += szofcur;
    write_args(it, std::forward<const Tail>(t)...);
  }

  template <class Iterator, typename Head>
  static void read_args(Iterator it, Head &&head) {
    auto szofcur = get_size_of(head);
    reader<Iterator, Head>::read_value(it, head);
    it += szofcur;
  }

  template <class Iterator, typename Head, typename... Tail>
  static void read_args(Iterator it, Head &&head, Tail &&... t) {
    auto szofcur = get_size_of(head);
    reader<Iterator, Head>::read_value(it, head);
    it += szofcur;
    read_args(it, std::forward<Tail>(t)...);
  }

public:
  static size_t capacity(const T &... args) {
    size_t result = 0;
    calculate_size_rec(result, std::forward<const T>(args)...);
    return result;
  }

  template <class Iterator> static void write(Iterator it, const T &... t) {
    write_args(it, std::forward<const T>(t)...);
  }

  template <class Iterator> static void read(Iterator it, T &... t) {
    read_args(it, std::forward<T>(t)...);
  }
};
} // namespace serialisation
} // namespace dqueue