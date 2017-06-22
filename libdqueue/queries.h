#pragma once

#include <libdqueue/kinds.h>
#include <libdqueue/network_message.h>
#include <libdqueue/serialisation.h>
#include <libdqueue/utils/utils.h>
#include <cstdint>
#include <cstring>

namespace dqueue {
namespace queries {

struct Ok {
  uint64_t id;

  using Scheme = serialisation::Scheme<uint64_t>;

  Ok(uint64_t id_) { id = id_; }

  Ok(const NetworkMessage_ptr &nd) { Scheme::read(nd->value(), id); }

  NetworkMessage_ptr toNetworkMessage() const {
    auto neededSize = Scheme::capacity(id);

    auto nd = std::make_shared<NetworkMessage>(
        neededSize, (NetworkMessage::message_kind)MessageKinds::OK);

    Scheme::write(nd->value(), id);
    return nd;
  }
};

struct CreateQueue {
  std::string name;

  using Scheme = serialisation::Scheme<std::string>;

  CreateQueue(const std::string &queue) { name = queue; }

  CreateQueue(const NetworkMessage_ptr &nd) { Scheme::read(nd->value(), name); }

  NetworkMessage_ptr toNetworkMessage() const {
    auto neededSize = Scheme::capacity(name);

    auto nd = std::make_shared<NetworkMessage>(
        neededSize, (NetworkMessage::message_kind)MessageKinds::CREATE_QUEUE);

    Scheme::write(nd->value(), name);
    return nd;
  }
};

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
  uint64_t messageId;
  std::vector<uint8_t> data;

  using Scheme = serialisation::Scheme<std::string, std::vector<uint8_t>, uint64_t>;

  Publish(const std::string &queue, const std::vector<uint8_t> &data_,
          uint64_t messageId_) {
    qname = queue;
    data = data_;
    messageId = messageId_;
  }

  Publish(const NetworkMessage_ptr &nd) {
    auto it = nd->value();
    Scheme::read(it, qname, data, messageId);
  }

  NetworkMessage_ptr toNetworkMessage() const {
    auto neededSize = Scheme::capacity(qname, data, messageId);
    auto nd = std::make_shared<NetworkMessage>(
        neededSize, (NetworkMessage::message_kind)MessageKinds::PUBLISH);

    Scheme::write(nd->value(), qname, data, messageId);
    return nd;
  }
};
} // namespace queries
} // namespace dqueue