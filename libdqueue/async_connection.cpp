#include <libdqueue/async_connection.h>
#include <libdqueue/utils/exception.h>
#include <functional>

using namespace std::placeholders;

using namespace boost::asio;

using namespace dqueue;

async_connection::async_connection(onDataRecvHandler onRecv, onNetworkErrorHandler onErr) {
  _async_con_id = 0;
  _messages_to_send = 0;
  _is_stoped = true;
  _on_recv_hadler = onRecv;
  _on_error_handler = onErr;
}

async_connection::~async_connection() noexcept(false) {
  full_stop();
}

void async_connection::start(const socket_ptr &sock) {
  if (!_is_stoped) {
    return;
  }
  _sock = sock;
  _is_stoped = false;
  _begin_stoping_flag = false;
  readNextAsync();
}

void async_connection::mark_stoped() {
  _begin_stoping_flag = true;
}

void async_connection::full_stop() {
  mark_stoped();
  try {
    if (auto spt = _sock.lock()) {
      if (spt->is_open()) {
        spt->close();
      }
    }
  } catch (...) {
  }
}

void async_connection::send(const network_message_ptr &d) {
  if (!_begin_stoping_flag) {
    auto ptr = shared_from_this();

    auto ds = d->as_buffer();
    auto send_buffer = std::get<1>(ds);
    auto send_buffer_size = std::get<0>(ds);

    if (auto spt = _sock.lock()) {
      _messages_to_send++;
      auto buf = buffer(send_buffer, send_buffer_size);
      async_write(*spt.get(), buf, [ptr, d](auto err, auto /*read_bytes*/) {
        ptr->_messages_to_send--;
        assert(ptr->_messages_to_send >= 0);
        if (err) {
          ptr->_on_error_handler(err);
        }
      });
    }
  }
}

void async_connection::readNextAsync() {
  if (auto spt = _sock.lock()) {
    auto ptr = shared_from_this();
	network_message_ptr d = std::make_shared<network_message>();

    async_read(*spt.get(), buffer((uint8_t *)(&d->size), SIZE_OF_MESSAGE_SIZE),
               [ptr, d, spt](auto err, auto read_bytes) {
                 if (err) {
                   if (err == boost::asio::error::operation_aborted ||
                       err == boost::asio::error::connection_reset ||
                       err == boost::asio::error::eof) {
                     return;
                   }
                   ptr->_on_error_handler(err);
                 } else {
                   if (read_bytes != SIZE_OF_MESSAGE_SIZE) {
                     THROW_EXCEPTION("exception on async readMarker. #",
                                     ptr->_async_con_id,
                                     " - wrong marker size: expected ",
                                     SIZE_OF_MESSAGE_SIZE, " readed ", read_bytes);
                   }
                   auto buf = buffer((uint8_t *)(&d->data), d->size);
                   async_read(*spt.get(), buf, [ptr, d](auto err, auto /*read_bytes*/) {
                     // logger("AsyncConnection::onReadData #", _async_con_id,
                     // "
                     // readed ", read_bytes);
                     if (err) {
                       ptr->_on_error_handler(err);
                     } else {
                       bool cancel_flag = false;
                       try {
                         ptr->_on_recv_hadler(d, cancel_flag);
                       } catch (std::exception &ex) {
                         THROW_EXCEPTION("exception on async readData. #",
                                         ptr->_async_con_id, " - ", ex.what());
                       }

                       if (!cancel_flag) {
                         ptr->readNextAsync();
                       }
                     }
                   });
                 }
               });
  }
}
