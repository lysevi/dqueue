#pragma once

#include <libdqueue/async_connection.h>
#include <libdqueue/exports.h>
#include <mutex>

namespace dqueue {

struct abstract_server {
  boost::asio::io_service *_service = nullptr;
  std::shared_ptr<boost::asio::ip::tcp::acceptor> _acc = nullptr;
  bool is_started = false;
  std::atomic_int  _next_id;

  struct params {
    unsigned short port;
  }; 

  struct io {
	  int   id;
	  socket_ptr sock = nullptr;
	  std::shared_ptr<async_connection> _async_connection = nullptr;

	  abstract_server *_server=nullptr;
	  io(int   id_,socket_ptr sock_, abstract_server*s) {
		  sock = sock_;
		  _server = s;
		  async_connection::onDataRecvHandler on_d =
			  [this](const network_message_ptr &d, bool &cancel) { onDataRecv(d, cancel); };
		  async_connection::onNetworkErrorHandler on_n =
			  [this](const boost::system::error_code &err) { onNetworkError(err); };
		  _async_connection = std::make_shared<async_connection>(on_d, on_n);
		  id = id_;
		  _async_connection->set_id(id);
		  _async_connection->start(sock);
	  }

	  void onNetworkError(const boost::system::error_code &err) {
		  //TODO call abstract virtual method.
	  }
	  
	  void onDataRecv(const network_message_ptr &d, bool &cancel) {
		  _server->onNewMessage(*this, d, cancel);
	  }
  };

  std::mutex _locker_connections;
  std::list<std::shared_ptr<io>> _connections;
  params _params;

  EXPORT abstract_server(boost::asio::io_service *service, params p);

  EXPORT void serverStart();

  EXPORT void start_accept(socket_ptr sock);
  EXPORT void handle_accept(socket_ptr sock, const boost::system::error_code &err);

  EXPORT virtual void onNewMessage(io& i,const network_message_ptr &d, bool &cancel) = 0;
};
}