#pragma once

#include "Direction.h"
#include "Point.h"
#include "bonuses/BonusTypeId.h"

#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/serialization/vector.hpp>

using boost::asio::ip::tcp;

class EventSystem;
//TODO: sync statistic
//TODO: start with pause in sync stage, then load level on client, then unpause(done sync stage)
struct Data final
{
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);

	int health{-1};
	int id{-1};
	BonusTypeId typeId{None};
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
	std::shared_ptr<EventSystem> _events{nullptr};

	Session(tcp::socket sock, std::shared_ptr<EventSystem> events);

	void Start();

	void DoRead();

	void DoWrite(const std::string& message);
};

struct Server final
{
	tcp::acceptor _acceptor;
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<std::shared_ptr<Session>> sessions;
	std::string _name;

	Server(boost::asio::io_context& ioContext, const std::string& host, const std::string& port,
	       std::shared_ptr<EventSystem> events);

	~Server();

	void DoAccept();

	void SendToAll(const std::string& message) const;
	void SendDispose(const std::string& bulletName) const;
	void SendKeyState(const std::string& state) const;
	void SendKeyState(const std::string& state, FPoint newPos) const;
	void SendShot(const std::string& objectName, Direction direction) const;
	void SendKeyState(const std::string& state, FPoint newPos, Direction direction) const;
	void SendPos(const std::string& objectName, FPoint newPos, Direction direction) const;
	void SendBonusSpawn(const std::string& objectName, FPoint spawnPos, BonusTypeId typeId, int id) const;
	void SendBonusDeSpawn(int id) const;
	auto SendHealth(const std::string& objectName, int health) const -> void;
	void SendTankDied(const std::string& objectName) const;
	void SendFortressDied(int id) const;
	void SendFortressToBrick(int id) const;
	void SendFortressToSteel(int id) const;
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

// Include the template implementation
#include "Server.tpp"
