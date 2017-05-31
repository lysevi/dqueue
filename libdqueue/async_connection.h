#pragma once

#include <libdqueue/network_message.h>
#include <libdqueue/socket_ptr.h>
#include <libdqueue/utils/async/locker.h>
#include <libdqueue/utils/exception.h>
#include <atomic>
#include <functional>
#include <memory>

namespace dqueue {
class async_connection : public std::enable_shared_from_this<async_connection> {
public:
  /// if method set 'cancel' to true, then read loop stoping.
  /// if dont_free_memory, then free NetData_ptr is in client side.
  using onDataRecvHandler = std::function<void(const network_message_ptr &d, bool &cancel)>;
  using onNetworkErrorHandler = std::function<void(const boost::system::error_code &err)>;

public:
  EXPORT async_connection(onDataRecvHandler onRecv, onNetworkErrorHandler onErr);
  EXPORT ~async_connection() noexcept(false);
  EXPORT void send(const network_message_ptr &d);
  EXPORT void start(const socket_ptr &sock);
  EXPORT void mark_stoped();
  EXPORT void full_stop(); /// stop thread, clean queue

  void set_id(int id) { _async_con_id = id; }
  int id() const { return _async_con_id; }
  int queue_size() const { return _messages_to_send; }

private:
  void readNextAsync();

private:
  std::atomic_int _messages_to_send;
  int _async_con_id;
  socket_weak _sock;

  bool _is_stoped;
  std::atomic_bool _begin_stoping_flag;

  onDataRecvHandler _on_recv_hadler;
  onNetworkErrorHandler _on_error_handler;
};
}
