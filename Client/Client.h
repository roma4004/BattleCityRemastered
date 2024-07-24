#pragma once

#include "../headers/Direction.h"
#include "../headers/EventSystem.h"
#include "../headers/Point.h"

#include <memory>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/serialization/vector.hpp>

class BaseObj;
using boost::asio::ip::tcp;

struct Data
{
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & health;
		ar & objectName;
		ar & eventName;
		ar & names;
		ar & newPos;
		ar & direction;
	}

	int health{-1};
	std::string objectName;
	std::string eventName;
	std::vector<std::string> names;
	FPoint newPos{0.f, 0.f};
	Direction direction{NONE};
};

class Client //: public std::enable_shared_from_this<Client>
{
public:
	Client(boost::asio::io_service& ioService, const std::string& host, const std::string& port,
	       std::shared_ptr<EventSystem> events);

	~Client();

	void ReadResponse();

	void SendKeyState(const std::string& state);

private:
	boost::asio::ip::tcp::socket _socket;
	boost::asio::streambuf _read_buffer;
	boost::asio::streambuf _write_buffer;
	std::shared_ptr<EventSystem> _events;
	std::string _name;
};
