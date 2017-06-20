#include "helpers.h"
#include <libdqueue/q.h>
#include <libdqueue/queries.h>
#include <libdqueue/serialisation.h>
#include <libdqueue/utils/utils.h>
#include <algorithm>

#include <catch.hpp>

using namespace dqueue;
using namespace dqueue::utils;
using namespace dqueue::queries;

TEST_CASE("serialisation.size_of_args") {
	EXPECT_EQ(serialisation::Scheme<int>::capacity(int(1)), sizeof(int));
	auto sz = serialisation::Scheme<int, int>::capacity(int(1), int(1));
	EXPECT_EQ(sz, sizeof(int) * 2);

	sz = serialisation::Scheme<int, int, double>::capacity(int(1), int(1), double(1.0));
	EXPECT_EQ(sz, sizeof(int) * 2 + sizeof(double));

	std::string str = "hello world";
	sz = serialisation::Scheme<std::string>::capacity(std::move(str));
	EXPECT_EQ(sz, sizeof(uint32_t) + str.size());
}

TEST_CASE("serialisation.scheme") {
	std::vector<int8_t> buffer(1024);

	auto it = buffer.begin();
	serialisation::Scheme<int>::write(it, 1);
	auto sz = std::distance(buffer.begin(), it);
	EXPECT_EQ(sz, sizeof(int));

	it = buffer.begin();
	serialisation::Scheme<int, int>::write(it, 1, 2);
	sz = std::distance(buffer.begin(), it);
	EXPECT_EQ(sz, sizeof(int) * 2);

	it = buffer.begin();
	int unpacked1, unpacked2;
	
	serialisation::Scheme<int, int>::read(it, unpacked1, unpacked2);
	EXPECT_EQ(unpacked1, 1);
	EXPECT_EQ(unpacked2, 2);
	
	it = buffer.begin();
	std::string str = "hello world";
	serialisation::Scheme<int, std::string>::write(it, 11, std::move(str));
	sz = std::distance(buffer.begin(), it);
	EXPECT_EQ(size_t(sz), size_t(sizeof(int) + sizeof(uint32_t) + str.size()));

	it = buffer.begin();
	std::string unpackedS;
	serialisation::Scheme<int, std::string>::read(it, unpacked1, unpackedS);
	EXPECT_EQ(unpacked1, 11);
	EXPECT_EQ(unpackedS, str); 
}

TEST_CASE("serialisation.queue_settings") {
	std::string name = "node.queue_settings.name";

	QueueSettings qs(name);
	EXPECT_EQ(qs.name, name);

	auto nd = qs.toNetworkMessage();
	EXPECT_EQ(nd->cast_to_header()->kind, 0);

	auto repacked = QueueSettings::fromNetworkMessage(nd);
	EXPECT_EQ(repacked.name.size(), name.size());
	EXPECT_EQ(repacked.name, name);
}

TEST_CASE("serialisation.subscribe") {
	std::string qname = "serialisation.subscribe";
	ChangeSubscribe cs{ qname };
	auto nd = cs.toNetworkMessage();
	EXPECT_EQ(nd->cast_to_header()->kind,
		(NetworkMessage::message_kind)MessageKinds::SUBSCRIBE);

	auto repacked = ChangeSubscribe(nd);
	EXPECT_EQ(repacked.qname, qname);
	EXPECT_EQ(repacked.qname.size(), qname.size());
}

TEST_CASE("serialisation.publish") {
	std::string qname = "serialisation.publish";
	std::vector<uint8_t> data = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	Publish pb{ qname, data };
	auto nd = pb.toNetworkMessage();
	EXPECT_EQ(nd->cast_to_header()->kind,
		(NetworkMessage::message_kind)MessageKinds::PUBLISH);

	auto repacked = Publish(nd);
	EXPECT_EQ(repacked.qname, qname);
	EXPECT_EQ(repacked.qname.size(), qname.size());

	EXPECT_EQ(repacked.data.size(), data.size());
	EXPECT_TRUE(
		std::equal(data.begin(), data.end(), repacked.data.begin(), repacked.data.end()));
}