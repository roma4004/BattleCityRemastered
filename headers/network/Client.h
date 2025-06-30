#pragma once

#include "Point.h"
#include "commands/Command.h"
#include <memory>
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/serialization/vector.hpp>

enum class BonusType : char8_t;
enum class Direction : char8_t;
class EventSystem;
class BaseObj;
using boost::asio::ip::tcp;

struct ClientData final
{
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive& ar, unsigned int version);

	std::string who{};
	std::string eventType{};
	std::string eventName{};
	std::string fraction{};
	std::vector<std::string> names{};
	FPoint pos{};
	int respawnResource{-1};
	int id{-1};
	int health{-1};
	BonusType type{};
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
	void OnPositionChange(const std::shared_ptr<Command>& command) const;
	void OnTankShot(const std::shared_ptr<Command>& command) const;
	void OnHealthChange(const std::shared_ptr<Command>& command) const;
	void OnDispose(const std::shared_ptr<Command>& command) const;
	void OnStatisticsChange(const std::shared_ptr<Command>& command) const;
	void OnKeyStateChange(const std::shared_ptr<Command>& command) const;
	void OnFortressChange(const std::shared_ptr<Command>& command) const;
	void OnBonusSpawn(const std::shared_ptr<Command>& command) const;
	void OnBonusDeSpawn(const std::shared_ptr<Command>& command) const;
	void OnRespawnTank(const std::shared_ptr<Command>& command) const;
	void OnObstacleSpawn(const std::shared_ptr<Command>& command) const;
	void OnCommandBatch(const std::shared_ptr<Command>& commands) const;
	void ProcessClientCommand(const std::shared_ptr<Command>& command) const;
	void ProcessReceivedData(const std::string& archiveData) const;

private:
	boost::asio::ip::tcp::socket _socket;
	boost::asio::streambuf _read_buffer{};
	boost::asio::streambuf _write_buffer{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::string _name{};
};

// Include the template implementation
#include "Client.tpp"
