#include <libdqueue/q.h>
#include <cstring>
#include <boost/date_time/posix_time/posix_time.hpp>

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

NetworkMessage_ptr QueueSettings::toNetworkMessage()const {
	uint32_t len = static_cast<uint32_t>(name.size());
	uint32_t neededSize = sizeof(uint32_t) + len;
	auto nd = std::make_shared<NetworkMessage>(neededSize, 0);
	memcpy(nd->value(), &(len), sizeof(uint32_t));
	memcpy(nd->value()+ sizeof(uint32_t), this->name.data(), this->name.size());
	return nd;
}

QueueSettings QueueSettings::fromNetworkMessage(const NetworkMessage_ptr&nd) {
	uint32_t len = 0;
	memcpy(&len, nd->value(), sizeof(uint32_t));
	ENSURE(len > uint32_t());

	std::string name;
	name.resize(len);
	memcpy(&name[0], nd->value() + sizeof(uint32_t), len);
	return QueueSettings(name);
}

void Queue::updateTime() {
  last_update = current_time();
}