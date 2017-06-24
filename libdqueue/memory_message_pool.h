#pragma once

#include <libdqueue/exports.h>
#include <libdqueue/imessage_pool.h>
#include <libdqueue/utils/utils.h>
#include <shared_mutex>
#include <unordered_map>

namespace dqueue {

class MemoryMessagePool : public IMessagePool {
public:
  MemoryMessagePool() {}
  ~MemoryMessagePool() {}
  void append(queries::Publish &pub) override {
    std::lock_guard<std::shared_mutex> sl(_locker);
    ENSURE(_pool.find(pub.messageId) == _pool.end());
    _pool.insert(std::make_pair(pub.messageId, pub));
  }

  void erase(uint64_t messageId) override {
    std::lock_guard<std::shared_mutex> sl(_locker);
    _pool.erase(messageId);
  }

  virtual std::vector<queries::Publish> all() const override {
    std::shared_lock<std::shared_mutex> sl(_locker);
    std::vector<queries::Publish> result;
    result.reserve(_pool.size());
    for (auto &kv : _pool) {
      result.emplace_back(kv.second);
    }
    return result;
  }

  size_t size() const {
    std::shared_lock<std::shared_mutex> sl(_locker);
    return _pool.size();
  }

protected:
  mutable std::shared_mutex _locker;
  std::unordered_map<uint64_t, queries::Publish> _pool;
};
} // namespace dqueue