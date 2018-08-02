#include <libdqueue/queries.h>
#include <benchmark/benchmark_api.h>

using namespace dqueue;
using namespace dqueue::queries;

class Serialisation : public benchmark::Fixture {
  virtual void SetUp(const ::benchmark::State &) {}

  virtual void TearDown(const ::benchmark::State &) {}

public:
};

BENCHMARK_DEFINE_F(Serialisation, Ok)(benchmark::State &state) {
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(Ok(uint64_t(1)).toNetworkMessage());
  }
}
BENCHMARK_REGISTER_F(Serialisation, Ok);

BENCHMARK_DEFINE_F(Serialisation, Login)(benchmark::State &state) {
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(Login("login").toNetworkMessage());
  }
}
BENCHMARK_REGISTER_F(Serialisation, Login);

BENCHMARK_DEFINE_F(Serialisation, LoginConfirm)(benchmark::State &state) {
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(LoginConfirm(uint64_t(1)).toNetworkMessage());
  }
}
BENCHMARK_REGISTER_F(Serialisation, LoginConfirm);
