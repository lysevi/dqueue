#pragma once

#include <libdqueue/kinds.h>
#include <libdqueue/network_message.h>
#include <libdqueue/utils/utils.h>
#include <cstdint>
#include <cstring>

namespace dqueue {
namespace queries {

struct ChangeSubscribe {
  std::string qname;
  
  ChangeSubscribe(const std::string &queue) { qname = queue; }

  ChangeSubscribe(const NetworkMessage_ptr &nd) {
    uint32_t len = 0;
    memcpy(&len, nd->value(), sizeof(uint32_t));
    ENSURE(len > uint32_t());

    qname.resize(len);
    memcpy(&qname[0], nd->value() + sizeof(uint32_t), len);
  }

  NetworkMessage_ptr toNetworkMessage() const {
    uint32_t len = static_cast<uint32_t>(qname.size());
    uint32_t neededSize = sizeof(uint32_t) + len;
    auto nd = std::make_shared<NetworkMessage>(
        neededSize, (NetworkMessage::message_kind)MessageKinds::SUBSCRIBE);

    memcpy(nd->value(), &(len), sizeof(uint32_t));
    memcpy(nd->value() + sizeof(uint32_t), this->qname.data(), this->qname.size());
    return nd;
  }
};

struct Publish {
  std::string qname;
  std::vector<uint8_t> data;

  Publish(const std::string &queue, const std::vector<uint8_t> &data_) {
    qname = queue;
    data = data_;
  }

  Publish(const NetworkMessage_ptr &nd) {
    uint32_t len = 0;

    uint8_t *ptr = nd->value();
    memcpy(&len, ptr, sizeof(uint32_t));
    ENSURE(len > uint32_t());
    ptr += sizeof(uint32_t);

    qname.resize(len);
    memcpy(&qname[0], ptr, len);
    ptr += len;

    memcpy(&len, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    data.resize(len);
    memcpy(&data[0], ptr, len);
  }

  NetworkMessage_ptr toNetworkMessage() const {
    uint32_t qname_len = static_cast<uint32_t>(qname.size());
    uint32_t data_len = static_cast<uint32_t>(data.size());
    uint32_t neededSize = sizeof(uint32_t) * 2 + qname_len + data_len;

    auto nd = std::make_shared<NetworkMessage>(
        neededSize, (NetworkMessage::message_kind)MessageKinds::PUBLISH);

    uint8_t *ptr = nd->value();
    memcpy(ptr, &(qname_len), sizeof(uint32_t));

    ptr += sizeof(uint32_t);
    memcpy(ptr, this->qname.data(), this->qname.size());

    ptr += this->qname.size();
    memcpy(ptr, &data_len, sizeof(uint32_t));

    ptr += sizeof(uint32_t);
    memcpy(ptr, &data[0], this->data.size());
    return nd;
  }
};
} // namespace queries
} // namespace dqueue