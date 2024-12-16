#pragma once

#include "Direction.h"
#include "Point.h"

#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <boost/serialization/vector.hpp>

using boost::asio::ip::tcp;

class EventSystem;

struct Data final
{
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive& ar, const unsigned int /*version*/)
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
	FPoint newPos{.x = 0.f, .y = 0.f};
	Direction direction{NONE};
};

struct Session final : public std::enable_shared_from_this<Session>
{
	tcp::socket _socket;
	boost::asio::streambuf _read_buffer;
	boost::asio::streambuf _write_buffer;
	std::shared_ptr<EventSystem> _events;

	Session(tcp::socket sock, std::shared_ptr<EventSystem> events);

	void Start();

	void DoRead();

	void DoWrite(const std::string& message);
};

struct Server final
{
	tcp::acceptor _acceptor;
	std::shared_ptr<EventSystem> _events;
	std::vector<std::shared_ptr<Session>> sessions;
	std::string _name;

	Server(boost::asio::io_service& ioService, const std::string& host, const std::string& port,
	       std::shared_ptr<EventSystem> events);

	~Server();

	void DoAccept();

	void SendToAll(const std::string& message) const;
	void SendKeyState(const std::string& state) const;
	void SendKeyState(const std::string& state, FPoint newPos) const;
	void SendShot(const std::string& state, Direction direction) const;
	void SendKeyState(const std::string& state, FPoint newPos, Direction direction) const;
	void SendNewPos(const std::string& objectName, const std::string& eventName, FPoint newPos,
	                Direction direction) const;
	void SendHealth(const std::string& objectName, const std::string& eventName, int health) const;

	// void stop() {
	// 	// Останавливаем принятие входящих подключений
	// 	acceptor_.close();
	//
	// 	// Закрываем все открытые сессии
	// 	for (auto& session : sessions) {
	// 		session->socket.close();
	// 	}
	//
	// 	// Очищаем список сессий
	// 	sessions.clear();
	//
	// 	// Останавливаем исполнитель io_service
	// 	_io_service.stop();
	// }
};
