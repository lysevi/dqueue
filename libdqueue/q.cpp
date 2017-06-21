#include <libdqueue/q.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <cstring>

const boost::posix_time::ptime START = boost::posix_time::from_time_t(0);
using namespace dqueue;

namespace {
uint64_t from_ptime(boost::posix_time::ptime timestamp) {
  auto duration = timestamp - START;
  auto ns = duration.total_milliseconds();
  return ns;
}

uint64_t current_time() {
  auto now = boost::posix_time::microsec_clock::universal_time();
  return from_ptime(now);
}
} // namespace

void Queue::updateTime() {
  last_update = current_time();
}