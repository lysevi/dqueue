#pragma once

#include <libdqueue/iqueue_client.h>
#include <libdqueue/kinds.h>
#include <libdqueue/network_message.h>
#include <libdqueue/node_settings.h>
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

struct Login {
  std::string login;

  using Scheme = serialisation::Scheme<std::string>;

  Login(const std::string &login_) { login = login_; }

  Login(const NetworkMessage_ptr &nd) { Scheme::read(nd->value(), login); }

  NetworkMessage_ptr toNetworkMessage() const {
    auto neededSize = Scheme::capacity(login);

    auto nd = std::make_shared<NetworkMessage>(
        neededSize, (NetworkMessage::message_kind)MessageKinds::LOGIN);

    Scheme::write(nd->value(), login);
    return nd;
  }
};

struct LoginConfirm {
  uint64_t id;

  using Scheme = serialisation::Scheme<uint64_t>;

  LoginConfirm(uint64_t id_) { id = id_; }

  LoginConfirm(const NetworkMessage_ptr &nd) { Scheme::read(nd->value(), id); }

  NetworkMessage_ptr toNetworkMessage() const {
    auto neededSize = Scheme::capacity(id);

    auto nd = std::make_shared<NetworkMessage>(
        neededSize, (NetworkMessage::message_kind)MessageKinds::LOGIN_CONFIRM);

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

  Publish(const PublishParams &settings, const std::vector<uint8_t> &data_,
          uint64_t messageId_) {
    qname = settings.queueName;
    data = data_;
    messageId = messageId_;
  }

  Publish(const std::string &qName, const std::vector<uint8_t> &data_,
          uint64_t messageId_) {
    qname = qName;
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

  MessageInfo toInfo() const {
    MessageInfo result;
    result.queueName = qname;
    return result;
  }

  PublishParams toPublishParams() {
    PublishParams result;
    result.queueName = qname;
    return result;
  }
};
} // namespace queries
} // namespace dqueue