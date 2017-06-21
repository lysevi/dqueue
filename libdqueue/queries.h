#pragma once

#include <libdqueue/kinds.h>
#include <libdqueue/network_message.h>
#include <libdqueue/serialisation.h>
#include <libdqueue/utils/utils.h>
#include <cstdint>
#include <cstring>

namespace dqueue {
namespace queries {

struct ChangeSubscribe {
  std::string qname;

  using Scheme = serialisation::Scheme<std::string>;

  ChangeSubscribe(const std::string &queue) { qname = queue; }

  ChangeSubscribe(const NetworkMessage_ptr &nd) { Scheme::read(nd->value(), qname); }

  NetworkMessage_ptr toNetworkMessage() const {
    auto neededSize = Scheme::capacity(qname);

    auto nd = std::make_shared<NetworkMessage>(
        neededSize, (NetworkMessage::message_kind)MessageKinds::SUBSCRIBE);

    Scheme::write(nd->value(), qname);
    return nd;
  }
};

struct Publish {
  std::string qname;
  std::vector<uint8_t> data;

  using Scheme = serialisation::Scheme<std::string, std::vector<uint8_t>>;

  Publish(const std::string &queue, const std::vector<uint8_t> &data_) {
    qname = queue;
    data = data_;
  }

  Publish(const NetworkMessage_ptr &nd) { 
	  auto it = nd->value();
	  Scheme::read(it, qname, data); 
  }

  NetworkMessage_ptr toNetworkMessage() const {
    auto neededSize = Scheme::capacity(qname, data);
    auto nd = std::make_shared<NetworkMessage>(
        neededSize, (NetworkMessage::message_kind)MessageKinds::PUBLISH);

    Scheme::write(nd->value(), qname, data);
    return nd;
  }
};
} // namespace queries
} // namespace dqueue