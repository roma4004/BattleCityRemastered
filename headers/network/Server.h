#pragma once

#include "../Point.h"

#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/serialization/vector.hpp>

enum Direction : char8_t;
enum BonusType : char8_t;
class EventSystem;

using boost::asio::ip::tcp;

//TODO: unpause when client connected(done sync stage)
struct ServerData final
{
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);

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

class Session final : public std::enable_shared_from_this<Session>
{
	tcp::socket _socket;
	boost::asio::streambuf _readBuffer;
	boost::asio::streambuf _writeBuffer;
	std::shared_ptr<EventSystem> _events{nullptr};

public:
	Session(tcp::socket sock, std::shared_ptr<EventSystem> events);

	~Session();

	void Start();

	void DoRead();

	void DoWrite(const std::string& message);
};

class Server final
{
	tcp::acceptor _acceptor;
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<std::shared_ptr<Session>> _sessions;
	std::string _name;

	void DoAccept();

	void SendToAll(const std::string& message) const;
	void SendDispose(const std::string& bulletName) const;
	void SendKeyState(const std::string& who) const;
	void SendShot(const std::string& who, Direction dir) const;
	void SendKeyState(const std::string& state, FPoint newPos, Direction dir) const;
	void SendPos(const std::string& who, FPoint pos, Direction dir) const;
	void SendBonusSpawn(const std::string& who, FPoint pos, BonusType type, int id) const;
	void SendBonusDeSpawn(int id) const;
	void SendHealth(const std::string& who, int health) const;
	void OnHelmetActivate(const std::string& who) const;
	void OnHelmetDeactivate(const std::string& who) const;
	void OnStar(const std::string& who) const;
	void OnTank(const std::string& who, const std::string& fraction) const;
	void OnGrenade(const std::string& who, const std::string& fraction) const;
	void OnStatisticsChange(const std::string& eventName, const std::string& author, const std::string& fraction) const;
	void SendFortressDied(int id) const;
	void SendFortressToBrick(int id) const;
	void SendFortressToSteel(int id) const;

public:
	Server(boost::asio::io_context& ioContext, const std::string& host, const std::string& port,
	       std::shared_ptr<EventSystem> events);

	~Server();

	void Subscribe() const;
	void SubscribeBonus() const;
	void Unsubscribe() const;
	void UnsubscribeBonus() const;
};

// Include the template implementation
#include "Server.tpp"
