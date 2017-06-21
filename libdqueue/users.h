#pragma once

#include <libdqueue/utils/utils.h>
#include <cstddef>
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <shared_mutex>

namespace dqueue {

	using Id = uint64_t;
	struct User {
		std::string login;
		Id id;
	};

	static const int ServerID = std::numeric_limits<int>::max();

	class UserBase;
	using UserBase_Ptr = std::shared_ptr<UserBase>;
	class UserBase {
	public:

		static UserBase_Ptr create() {
			return std::make_shared<UserBase>();
		}

		void append(const User&user) {
			std::lock_guard<std::shared_mutex> sl(_locker);
			logger_info("node: add client #", user.id);
			_users[user.id] = user;
		}

		void erase(Id id) {
			std::lock_guard<std::shared_mutex> sl(_locker);
			logger_info("node: erase client #", id);
			_users.erase(id);
		}

		bool exists(Id id) const {
			std::shared_lock<std::shared_mutex> sl(_locker);
			auto it = _users.find(id);
			return it != _users.end();
		}

		bool byId(Id id, User&output) const {
			std::shared_lock<std::shared_mutex> sl(_locker);
			auto it = _users.find(id);
			if (it == _users.end()) {
				return false;
			}
			else {
				output = it->second;
				return true;
			}
		}

		std::vector<User> users() const {
			std::shared_lock<std::shared_mutex> sl(_locker);
			std::vector<User> result(_users.size());
			auto it = result.begin();
			for (auto kv : _users) {
				*it = kv.second;
				++it;
			}
			return result;
		}
	protected:
		mutable std::shared_mutex _locker;
		std::unordered_map<Id, User> _users;
	};
}