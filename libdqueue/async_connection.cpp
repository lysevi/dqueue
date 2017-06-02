#include <libdqueue/async_connection.h>
#include <libdqueue/utils/exception.h>
#include <functional>

using namespace std::placeholders;

using namespace boost::asio;

using namespace dqueue;

AsyncConnection::AsyncConnection(onDataRecvHandler onRecv, onNetworkErrorHandler onErr) {
  _async_con_id = 0;
  _messages_to_send = 0;
  _is_stoped = true;
  _on_recv_hadler = onRecv;
  _on_error_handler = onErr;
}

AsyncConnection::~AsyncConnection() noexcept(false) {
  full_stop();
}

void AsyncConnection::start(const socket_ptr &sock) {
  if (!_is_stoped) {
    return;
  }
  _sock = sock;
  _is_stoped = false;
  _begin_stoping_flag = false;
  readNextAsync();
}

void AsyncConnection::mark_stoped() {
  _begin_stoping_flag = true;
}

void AsyncConnection::full_stop() {
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

void AsyncConnection::send(const NetworkMessage_ptr &d) {
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
        }
      });
    }
  }
}

void AsyncConnection::readNextAsync() {
  if (auto spt = _sock.lock()) {
    auto ptr = shared_from_this();
    NetworkMessage_ptr d = std::make_shared<NetworkMessage>();

	async_read(*spt.get(), buffer((uint8_t *)(&d->size), SIZE_OF_MESSAGE_SIZE),
		[ptr, d, spt](auto err, auto read_bytes) {
		if (err) {
			ptr->_on_error_handler(d, err);
		}
		else {
			if (read_bytes != SIZE_OF_MESSAGE_SIZE) {
				THROW_EXCEPTION("exception on async readMarker. #",
					ptr->_async_con_id,
					" - wrong marker size: expected ",
					SIZE_OF_MESSAGE_SIZE, " readed ", read_bytes);
			}
			auto buf = buffer((uint8_t *)(&d->data), d->size);
			async_read(*spt.get(), buf, [ptr, d](auto err, auto /*read_bytes*/) {
				if (err) {
					ptr->_on_error_handler(d, err);
				}
				else {
					bool cancel_flag = false;
					try {
						ptr->_on_recv_hadler(d, cancel_flag);
					}
					catch (std::exception &ex) {
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
