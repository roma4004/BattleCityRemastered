#pragma once

#include "../../headers/Point.h"

#include <memory>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/serialization/vector.hpp>

enum BonusType : char8_t;
class EventSystem;
enum Direction : char8_t;
class BaseObj;
using boost::asio::ip::tcp;

struct ClientData final
{
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive& ar, unsigned int version);

	int health{-1};
	int respawnResource{-1};
	int id{-1};
	BonusType type{};
	std::string who;
	std::string eventType;
	std::string eventName;
	std::string fraction;
	std::vector<std::string> names;
	FPoint pos{};
	Direction dir{};
};

class Client final//: public std::enable_shared_from_this<Client>
{
public:
	Client(boost::asio::io_context& ioContext, const std::string& host, const std::string& port,
	       std::shared_ptr<EventSystem> events);

	~Client();

	void Subscribe();
	void Unsubscribe() const;

	void ReadResponse();

	void SendKeyState(const std::string& state);

private:
	boost::asio::ip::tcp::socket _socket;
	boost::asio::streambuf _read_buffer;
	boost::asio::streambuf _write_buffer;
	std::shared_ptr<EventSystem> _events{nullptr};
	std::string _name;
};

// Include the template implementation
#include "Client.tpp"
