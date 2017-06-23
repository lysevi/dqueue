#include <libdqueue/client.h>
#include <libdqueue/server.h>
#include <libdqueue/utils/utils.h>
#include <cxxopts.hpp>
#include <iostream>
#include <unordered_map>

class BenchmarkLogger : public dqueue::utils::ILogger {
public:
  static bool verbose;
  BenchmarkLogger() {}
  ~BenchmarkLogger() {}

  void message(dqueue::utils::LOG_MESSAGE_KIND kind, const std::string &msg) {
    if (!verbose) {
      return;
    }
    std::stringstream ss;
    switch (kind) {
    case dqueue::utils::LOG_MESSAGE_KIND::FATAL:
      ss << "[err] " << msg << std::endl;
      break;
    case dqueue::utils::LOG_MESSAGE_KIND::INFO:
      ss << "[inf] " << msg << std::endl;
      break;
    case dqueue::utils::LOG_MESSAGE_KIND::MESSAGE:
      ss << "[dbg] " << msg << std::endl;
      break;
    }
    if (kind == dqueue::utils::LOG_MESSAGE_KIND::FATAL) {
      std::cerr << ss.str();
    } else {
      std::cout << ss.str();
    }
  }
};

bool BenchmarkLogger::verbose = false;
bool run_server = true;
size_t clients_count = 1;
size_t server_threads = 2;
size_t queue_count = 1;

std::shared_ptr<dqueue::Server> server;
std::unique_ptr<boost::asio::io_service> server_service;
bool server_thread_stop = false;
void server_thread() {
  while (!server_thread_stop) {
    server_service->run();
  }
}

static std::atomic_int sended;
bool show_info_stop = false;
void show_info_thread() {
  uint64_t lastSended = sended.load();
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto curSended = sended.load();

    auto diff = curSended - lastSended;
    std::cout << "speed: " << diff << std::endl;
    lastSended = curSended;
    if (show_info_stop) {
      break;
    }
  }
}

class BenchmarkClient : public dqueue::Client {
public:
  BenchmarkClient() = delete;
  BenchmarkClient(boost::asio::io_service *service, const AbstractClient::Params &_params)
      : dqueue::Client(service, _params) {}

  void onConnect() override {
    dqueue::Client::onConnect();
    for (size_t j = 0; j < queue_count; ++j) {
      auto qname = "serverQ_" + std::to_string(j);
      subscribe(qname);
	  publish(qname, { 1 });
    }
  }

  void onMessage(const std::string &queueName, const dqueue::rawData &d) override {
    if (messagesInPool() < size_t(5)) {
      publish(queueName, d);
    }
  }
};

int main(int argc, char *argv[]) {
  auto logger = dqueue::utils::ILogger_ptr{new BenchmarkLogger};
  dqueue::utils::LogManager::start(logger);

  cxxopts::Options options("benchmark_sum", "benchmark for queue and many consumers");
  options.positional_help("[optional args]");
  auto opts = options.add_options();
  opts("help", "Print help");
  opts("D,debug", "Enable debugging.", cxxopts::value<bool>(BenchmarkLogger::verbose));
  opts("Q,queues", "queues count.", cxxopts::value<size_t>(queue_count));
  opts("S,server-threads", "queues count.", cxxopts::value<size_t>(server_threads));
  opts("dont-run-server", "run server.");
  opts("clients", "clients", cxxopts::value<size_t>(clients_count));

  options.parse(argc, argv);

  if (options.count("help")) {
    std::cout << options.help() << std::endl;
    std::exit(0);
  }
  if (options.count("dont-run-server")) {
    run_server = false;
  }
  server_service = std::make_unique<boost::asio::io_service>();

  dqueue::AbstractServer::params p;
  p.port = 4040;
  dqueue::DataHandler server_handler = [](const std::string &queueName,
                                          const dqueue::rawData &d, dqueue::Id id) {
    server->publish(queueName, d);
    sended++;
  };

  if (run_server) {
    server = std::make_shared<dqueue::Server>(server_service.get(), p);
    server->serverStart();
    server->addHandler(server_handler);
  }

  std::thread show_thread;

  std::list<std::thread> threads;
  for (size_t i = 0; i < server_threads; ++i) {
    dqueue::logger("start thread #", i);
    threads.emplace_back(&server_thread);
  }

  if (run_server) {
    for (size_t i = 0; i < queue_count; ++i) {
      dqueue::QueueSettings qs("serverQ_" + std::to_string(i));
      server->createQueue(qs);
      server->subscribe(qs.name);
    }
  }

  std::list<std::shared_ptr<BenchmarkClient>> clients;

  for (size_t i = 0; i < clients_count; ++i) {
    dqueue::AbstractClient::Params client_param("client_" + std::to_string(i),
                                                "localhost", 4040);
    auto cl = std::make_shared<BenchmarkClient>(server_service.get(), client_param);
    dqueue::logger_info("client ", i, " start connection");
    cl->async_connect();
    dqueue::logger_info("client ", i, " was connected");
    clients.push_back(cl);
    dqueue::logger("client #", i, " connected");
  }

  if (run_server) {
    show_thread = std::move(std::thread{show_info_thread});
  }

  /*for (auto cl : clients) {
    for (size_t j = 0; j < queue_count; ++j) {
      auto qname = "serverQ_" + std::to_string(j);
      cl->publish(qname, {1});
    }
  }*/

  size_t pos = 0;
  for (auto &it : threads) {
    dqueue::logger("whait thread #", pos);
    it.join();
    ++pos;
  }

  if (run_server) {
    show_info_stop = true;
    show_thread.join();
  }
}