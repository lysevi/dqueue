#include <libdqueue/q.h>
#include <libdqueue/serialisation.h>
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
using QueueSettingsScheme = serialisation::Scheme<std::string>;
} // namespace

NetworkMessage_ptr QueueSettings::toNetworkMessage() const {
  auto neededSize = QueueSettingsScheme::capacity(name);
  auto nd = std::make_shared<NetworkMessage>(neededSize, 0);
  QueueSettingsScheme::write(nd->value(), name);
  return nd;
}

QueueSettings QueueSettings::fromNetworkMessage(const NetworkMessage_ptr &nd) {
  std::string name;
  QueueSettingsScheme::read(nd->value(), name);
  return QueueSettings(name);
}

void Queue::updateTime() {
  last_update = current_time();
}