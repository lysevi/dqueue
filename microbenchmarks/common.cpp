#include "common.h"

namespace microbenchmark_common {
void replace_std_logger() {
  auto _raw_ptr = new BenchmarkLogger();
  auto _logger = dqueue::utils::ILogger_ptr{_raw_ptr};

  dqueue::utils::LogManager::start(_logger);
}
} // namespace microbenchmark_common