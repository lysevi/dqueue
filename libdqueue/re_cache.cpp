#include <libdqueue/re_cache.h>

using namespace dqueue;

std::unique_ptr<ReCache> ReCache::_instance = nullptr;
std::mutex ReCache::_instance_locker;