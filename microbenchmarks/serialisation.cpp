#include <libdqueue/queries.h>
#include <benchmark/benchmark_api.h>

using namespace dqueue;
using namespace dqueue::queries;

class Serialisation : public benchmark::Fixture {
  virtual void SetUp(const ::benchmark::State &) {
    const std::string qname = "Serialisation.queue";
    _ChangeSubscribe = std::make_unique<ChangeSubscribe>(qname);
    _Publish = std::make_unique<Publish>(
        qname, std::vector<uint8_t>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  }
  virtual void TearDown(const ::benchmark::State &) {}

public:
  std::unique_ptr<ChangeSubscribe> _ChangeSubscribe;
  std::unique_ptr<Publish> _Publish;
};

BENCHMARK_DEFINE_F(Serialisation, ChangeSubscribeFrom)(benchmark::State &state) {
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(_ChangeSubscribe->toNetworkMessage());
  }
}
BENCHMARK_REGISTER_F(Serialisation, ChangeSubscribeFrom);

BENCHMARK_DEFINE_F(Serialisation, ChangeSubscribeTo)(benchmark::State &state) {
  auto nd = _ChangeSubscribe->toNetworkMessage();
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(ChangeSubscribe(nd));
  }
}
BENCHMARK_REGISTER_F(Serialisation, ChangeSubscribeTo);

BENCHMARK_DEFINE_F(Serialisation, PublishFrom)(benchmark::State &state) {
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(_Publish->toNetworkMessage());
  }
}
BENCHMARK_REGISTER_F(Serialisation, PublishFrom);

BENCHMARK_DEFINE_F(Serialisation, PublishTo)(benchmark::State &state) {
  auto nd = _Publish->toNetworkMessage();
  while (state.KeepRunning()) {
    benchmark::DoNotOptimize(Publish(nd));
  }
}
BENCHMARK_REGISTER_F(Serialisation, PublishTo);