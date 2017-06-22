#pragma once

#include <libdqueue/exports.h>
#include <libdqueue/queries.h>
#include <memory>

namespace dqueue {

class IMessagePool {
public:
  ~IMessagePool() {}
  virtual void append(queries::Publish &pub) = 0;
  virtual void erase(uint64_t messageId) = 0;
  virtual std::vector<queries::Publish> all() const = 0;
  virtual size_t size() const = 0;
};
using MessagePool_Ptr = std::shared_ptr<IMessagePool>;
} // namespace dqueue