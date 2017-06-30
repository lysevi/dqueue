#pragma once

#include <libdqueue/exports.h>
#include <libdqueue/utils/utils.h>
#include <memory>
#include <mutex>
#include <regex>
#include <string>
#include <unordered_map>

namespace dqueue {
struct ReCache {
  static std::unique_ptr<ReCache> _instance;
  static std::mutex _instance_locker;
  EXPORT static ReCache *instance() {
    if (ReCache::_instance == nullptr) {
      std::lock_guard<std::mutex> lg(_instance_locker);
      if (ReCache::_instance == nullptr) {
        _instance = std::make_unique<ReCache>();
      }
    }
    return _instance.get();
  }

  std::shared_ptr<std::regex> compile(const std::string &re) {
    std::lock_guard<std::mutex> lg(_locker);
    auto it = _cache.find(re);
    if (it == _cache.end()) {
      auto cre = std::make_shared<std::regex>(re);
      _cache.insert(std::make_pair(re, cre));
      return cre;
    }
    return it->second;
  }

  std::unordered_map<std::string, std::shared_ptr<std::regex>> _cache;
  mutable std::mutex _locker;
};
} // namespace dqueue