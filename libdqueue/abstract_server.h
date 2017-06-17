#pragma once

#include <libdqueue/async_io.h>
#include <libdqueue/exports.h>
#include <mutex>

namespace dqueue {

class AbstractServer : public std::enable_shared_from_this<AbstractServer> {
public:
  struct params {
    unsigned short port;
  };

  enum class ON_NEW_CONNECTION_RESULT { ACCEPT, DISCONNECT };

  class ClientConnection {
  public:
    ClientConnection(int id_, socket_ptr sock_, std::shared_ptr<AbstractServer> s);
    ~ClientConnection();
    void onMessageSended(const NetworkMessage_ptr &d);
    void onNetworkError(const NetworkMessage_ptr &d,
                        const boost::system::error_code &err);
    void onDataRecv(const NetworkMessage_ptr &d, bool &cancel);
    EXPORT void sendData(const NetworkMessage_ptr &d);
    EXPORT int get_id() const { return id; }

  private:
    int id;
    socket_ptr sock = nullptr;
    std::shared_ptr<AsyncIO> _async_connection = nullptr;
    std::shared_ptr<AbstractServer> _server = nullptr;
  };

  EXPORT AbstractServer(boost::asio::io_service *service, params p);
  EXPORT virtual ~AbstractServer();
  EXPORT void serverStart();
  EXPORT void stopServer();
  EXPORT void start_accept(socket_ptr sock);
  EXPORT bool is_started() const { return _is_started; }
  EXPORT bool is_stoped() const { return _is_stoped; }

  virtual void onMessageSended(ClientConnection &i, const NetworkMessage_ptr &d) = 0;
  virtual void onNetworkError(ClientConnection &i, const NetworkMessage_ptr &d,
                              const boost::system::error_code &err) = 0;
  virtual void onNewMessage(ClientConnection &i, const NetworkMessage_ptr &d,
                            bool &cancel) = 0;
  virtual ON_NEW_CONNECTION_RESULT onNewConnection(ClientConnection &i) = 0;

private:
  static void handle_accept(std::shared_ptr<AbstractServer> self, socket_ptr sock,
                            const boost::system::error_code &err);
  void disconnect_client(const ClientConnection*client);

protected:
  boost::asio::io_service *_service = nullptr;
  std::shared_ptr<boost::asio::ip::tcp::acceptor> _acc = nullptr;
  bool _is_started = false;
  std::atomic_int _next_id;
  std::mutex _locker_connections;
  std::list<std::shared_ptr<ClientConnection>> _connections;
  params _params;
  bool _is_stoped = false;
};
}
