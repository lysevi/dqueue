#include <libdqueue/async_io.h>
#include <libdqueue/utils/exception.h>
#include <functional>

using namespace std::placeholders;
using namespace boost::asio;
using namespace dqueue;

AsyncIO::AsyncIO(onDataRecvHandler onRecv, onNetworkErrorHandler onErr,
                 onNetworkSuccessSendHandler onSended) {
  _async_con_id = 0;
  _messages_to_send = 0;
  _is_stoped = true;
  _on_recv_hadler = onRecv;
  _on_error_handler = onErr;
  _on_sended_handler = onSended;
}

AsyncIO::~AsyncIO() noexcept(false) {
  full_stop();
}

void AsyncIO::start(const socket_ptr &sock) {
  if (!_is_stoped) {
    return;
  }
  _sock = sock;
  _is_stoped = false;
  _begin_stoping_flag = false;
  readNextAsync();
}

void AsyncIO::mark_stoped() {
  _begin_stoping_flag = true;
}

void AsyncIO::full_stop() {
  // mark_stoped();
  try {
    if (auto spt = _sock.lock()) {
      if (spt->is_open()) {
        spt->close();
      }
    }
  } catch (...) {
  }
}

void AsyncIO::send(const NetworkMessage_ptr &d) {
  if (!_begin_stoping_flag) {
    auto ptr = shared_from_this();

    auto ds = d->as_buffer();
    auto send_buffer = std::get<1>(ds);
    auto send_buffer_size = std::get<0>(ds);

    if (auto spt = _sock.lock()) {
      _messages_to_send++;
      auto buf = buffer(send_buffer, send_buffer_size);
      async_write(*spt.get(), buf, [ptr, d](auto err, auto /*read_bytes*/) {
        if (err) {
          ptr->_on_error_handler(d, err);
        } else {
          ptr->_messages_to_send--;
          assert(ptr->_messages_to_send >= 0);
          ptr->_on_sended_handler(d);
        }
      });
    }
  }
}

void AsyncIO::readNextAsync() {
  if (auto spt = _sock.lock()) {
    auto ptr = shared_from_this();

    auto on_read_message = [ptr](NetworkMessage_ptr d, auto err, auto /*read_bytes*/) {
      if (err) {
        ptr->_on_error_handler(d, err);
      } else {
        bool cancel_flag = false;
        try {
          ptr->_on_recv_hadler(d, cancel_flag);
        } catch (std::exception &ex) {
          THROW_EXCEPTION("exception on async readData. #", ptr->_async_con_id, " - ",
                          ex.what());
        }

        if (!cancel_flag) {
          ptr->readNextAsync();
        }
      }
    };

    auto on_read_size = [this, ptr, spt, on_read_message](auto err, auto read_bytes) {
      if (err) {
        ptr->_on_error_handler(nullptr, err);
      } else {
        if (read_bytes != NetworkMessage::SIZE_OF_MESSAGE_SIZE) {
          THROW_EXCEPTION("exception on async readMarker. #", ptr->_async_con_id,
                          " - wrong marker size: expected ",
                          NetworkMessage::SIZE_OF_MESSAGE_SIZE, " readed ", read_bytes);
        }

		auto data_left = ptr->next_message_size - NetworkMessage::SIZE_OF_MESSAGE_SIZE;
        NetworkMessage_ptr d = std::make_shared<NetworkMessage>(data_left);
        auto buf_ptr = (uint8_t *)(d->data + NetworkMessage::SIZE_OF_MESSAGE_SIZE);
        auto buf = buffer(buf_ptr, data_left);
        async_read(*spt.get(), buf, std::bind(on_read_message, d, _1, _2));
      }
    };

    async_read(
        *spt.get(),
        buffer((void *)&(ptr->next_message_size), NetworkMessage::SIZE_OF_MESSAGE_SIZE),
        on_read_size);
  }
}
