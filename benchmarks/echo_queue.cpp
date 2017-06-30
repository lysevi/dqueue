#include <libdqueue/dqueue.h>
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
    switch (kind) {
    case dqueue::utils::LOG_MESSAGE_KIND::FATAL:
      std::cerr << "[err] " << msg << std::endl;
      break;
    case dqueue::utils::LOG_MESSAGE_KIND::INFO:
      std::cout << "[inf] " << msg << std::endl;
      break;
    case dqueue::utils::LOG_MESSAGE_KIND::MESSAGE:
      //std::cout << "[dbg] " << msg << std::endl;
      break;
    }
  }
};

bool BenchmarkLogger::verbose = false;
bool run_server = true;
size_t clients_count = 1;
size_t queue_count = 1;
int maxSends = 100000;

std::unique_ptr<boost::asio::io_service> server_service;

static std::atomic_int server_received;
static std::atomic_int client_sended;
bool show_info_stop = false;

class BenchmarkServer final: public virtual dqueue::Server, public dqueue::EventConsumer {
public:
  BenchmarkServer(boost::asio::io_service *service, dqueue::AbstractServer::Params p)
      : dqueue::Server(service, p) {}

  void queueHandler(const dqueue::PublishParams &info, const dqueue::rawData &d,
                    dqueue::Id) {
    publish(dqueue::PublishParams(info.queueName, "server"), d);
    server_received.fetch_add(1);
  }

  void onStartComplete() override {
    dqueue::Server::onStartComplete();

    for (size_t i = 0; i < queue_count; ++i) {
      dqueue::QueueSettings qs("serverQ_" + std::to_string(i));
      createQueue(qs);
      dqueue::SubscriptionParams sp(qs.name, "client");
      subscribe(sp, this);
    }
  }

  void consume(const dqueue::PublishParams &info, const dqueue::rawData &d,
               dqueue::Id) override {
    this->publish(dqueue::PublishParams(info.queueName, "server"), d);
    server_received.fetch_add(1);
  }
};

std::shared_ptr<BenchmarkServer> server;

void server_thread() {
  dqueue::AbstractServer::Params p;
  p.port = 4040;

  server_service = std::make_unique<boost::asio::io_service>();

  server = std::make_shared<BenchmarkServer>(server_service.get(), p);
  server->serverStart();

  bool srv = false;
  bool clnt = false;
  while (true) {
    server_service->run_one();
    srv = server_received.load() > maxSends;
    clnt = int(client_sended.load() / clients_count) > maxSends;
    if (srv && clnt) {
      break;
    }
  }
}

class BenchmarkClient  final: public dqueue::Client, public dqueue::EventConsumer {
public:
  BenchmarkClient() = delete;
  BenchmarkClient(boost::asio::io_service *service, const AbstractClient::Params &_params)
      : dqueue::Client(service, _params) {}

  void onConnect() override {
    dqueue::logger("benchmark_client #", dqueue::Client::getId(), " connected.");
    dqueue::Client::onConnect();
    for (size_t j = 0; j < queue_count; ++j) {
      auto qname = "serverQ_" + std::to_string(j);
      dqueue::SubscriptionParams sp(qname, "server");
      dqueue::logger("benchmark_client #", dqueue::Client::getId(), " subscribe to ",
                     qname);
      subscribe(sp, this, dqueue::OperationType::Async);
      publish(dqueue::PublishParams(qname, "client"), {1}, dqueue::OperationType::Async);
    }
  }

  void consume(const dqueue::PublishParams &info, const dqueue::rawData &d,
               dqueue::Id) override {
    // if (messagesInPool() < size_t(5))
    {
      publish(dqueue::PublishParams(info.queueName, "client"), d,
              dqueue::OperationType::Async);
      client_sended.fetch_add(1);
    }
  }
};

void client_thread(std::shared_ptr<boost::asio::io_service> client_service,
                   std::shared_ptr<BenchmarkClient> client) {
  dqueue::logger_info("client ", client->getParams().login, " start connection");
  client->async_connect();
  bool srv = false;
  bool clnt = false;
  while (true) {
    client_service->run_one();
    srv = server_received.load() > maxSends;
    clnt = int(client_sended.load() / clients_count) > maxSends;
    if (srv && clnt) {
      break;
    }
  }
}

void show_info_thread() {
  uint64_t lastRecv = server_received.load();
  uint64_t lastSend = client_sended.load();
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto curRecv = server_received.load();
    auto curSend = client_sended.load();

    auto diffRecv = curRecv - lastRecv;
    auto diffSend = curSend - lastSend;
    if (run_server) {
      std::cout << " recv: " << 100.0 * curRecv / float(maxSends) << "% ";
    }
    if (clients_count != 0) {
      std::cout << " send: " << 100.0 * curSend / float(maxSends) << "% ";
    }

    if (run_server) {
      std::cout << " (recv: " << diffRecv << "/sec. - " << curRecv << ")";
    }
    if (clients_count != 0) {
      std::cout << " (send: " << diffSend / clients_count << "/sec. : " << curSend << ")";
    }
    std::cout << std::endl;
    lastRecv = curRecv;
    lastSend = curSend;
    if (show_info_stop) {
      break;
    }
  }
}

int main(int argc, char *argv[]) {
  auto logger = dqueue::utils::ILogger_ptr{new BenchmarkLogger};
  dqueue::utils::LogManager::start(logger);

  cxxopts::Options options("benchmark_sum", "benchmark for queue and many consumers");
  options.positional_help("[optional args]");
  auto opts = options.add_options();
  opts("help", "Print help");
  opts("D,debug", "Enable debugging.", cxxopts::value<bool>(BenchmarkLogger::verbose));
  opts("Q,queues", "queues count.", cxxopts::value<size_t>(queue_count));
  opts("dont-run-server", "dont run server.");
  opts("clients", "clients", cxxopts::value<size_t>(clients_count));
  opts("maxSends", "maximum sends", cxxopts::value<int>(maxSends));

  try {
    options.parse(argc, argv);
  } catch (cxxopts::option_not_exists_exception &exx) {
    std::cerr << "cmd line error: " << exx.what() << std::endl;
    return 1;
  }

  if (options.count("help")) {
    std::cout << options.help() << std::endl;
    std::exit(0);
  }

  if (options.count("dont-run-server")) {
    run_server = false;
  }

  std::thread show_thread;

  std::list<std::thread> threads;

  if (run_server) {
    threads.emplace_back(&server_thread);

    while (server == nullptr || !server->is_started()) {
      dqueue::logger("wait server...");
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
  }

  std::list<std::shared_ptr<BenchmarkClient>> clients;

  for (size_t i = 0; i < clients_count; ++i) {
    dqueue::logger("start client thread #", i);
    dqueue::AbstractClient::Params client_param("client-" + std::to_string(i),
                                                "localhost", 4040);
    std::shared_ptr<boost::asio::io_service> client_service(new boost::asio::io_service);
    auto cl = std::make_shared<BenchmarkClient>(client_service.get(), client_param);
    clients.push_back(cl);
    threads.emplace_back(&client_thread, client_service, cl);
  }

  show_thread = std::move(std::thread{show_info_thread});

  size_t pos = 0;
  for (auto &it : threads) {
    dqueue::logger("whait thread #", pos);
    it.join();
    ++pos;
  }

  show_info_stop = true;
  show_thread.join();
}
