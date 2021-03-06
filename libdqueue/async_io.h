#pragma once

#include <libdqueue/network_message.h>
#include <libdqueue/socket_ptr.h>
#include <libdqueue/utils/async/locker.h>
#include <libdqueue/utils/exception.h>
#include <atomic>
#include <functional>
#include <memory>

namespace dqueue {
class AsyncIO : public std::enable_shared_from_this<AsyncIO> {
public:
  /// if method set 'cancel' to true, then read loop stoping.
  /// if dont_free_memory, then free NetData_ptr is in client side.
  using onDataRecvHandler =
      std::function<void(const NetworkMessage_ptr &d, bool &cancel)>;
  using onNetworkErrorHandler = std::function<void(const NetworkMessage_ptr &d,
                                                   const boost::system::error_code &err)>;

  EXPORT AsyncIO(onDataRecvHandler onRecv, onNetworkErrorHandler onErr);
  EXPORT ~AsyncIO() noexcept(false);
  EXPORT void send(const NetworkMessage_ptr d);
  EXPORT void start(const socket_ptr &sock);
  EXPORT void full_stop(); /// stop thread, clean queue

  int queue_size() const { return _messages_to_send; }

private:
  void readNextAsync();

private:
  std::atomic_int _messages_to_send;
  socket_weak _sock;

  bool _is_stoped;
  std::atomic_bool _begin_stoping_flag;
  NetworkMessage::message_size next_message_size;

  onDataRecvHandler _on_recv_hadler;
  onNetworkErrorHandler _on_error_handler;
};
} // namespace dqueue
