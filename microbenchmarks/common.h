#pragma once
#include <libdqueue/utils/logger.h>
namespace microbenchmark_common {
class BenchmarkLogger : public dqueue::utils::ILogger {
public:
  BenchmarkLogger() {}
  ~BenchmarkLogger() {}
  void message(dqueue::utils::LOG_MESSAGE_KIND, const std::string &) {}
};

void replace_std_logger();
} // namespace microbenchmark_common