#pragma once

#include "MessageFraming.h"
#include <array>
#include <boost/asio/ip/tcp.hpp>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace network
{
using boost::asio::ip::tcp;

class FrameChannel final : public std::enable_shared_from_this<FrameChannel>
{
public:
	using FrameHandler = std::function<void(const std::string&)>;
	using ErrorHandler = std::function<void()>;
	using DrainHandler = std::function<void()>;

	FrameChannel(tcp::socket socket, std::string ownerName);

	//NOTE: handlers capture the owner, so the owner must call Close() before it dies
	void SetHandlers(FrameHandler onFrame, ErrorHandler onError);

	void StartReading();
	void Send(std::shared_ptr<const std::string> frame);
	void Close();

	void CloseForReconnect();
	void CloseAfterFlush(DrainHandler onClosed);
	void SetWriteEnabled(bool enabled);

	[[nodiscard]] bool IsOpen() const { return _socket.is_open(); }
	[[nodiscard]] tcp::socket& Socket() { return _socket; }

private:
	void ReadHeader();
	void ReadPayload(std::uint32_t payloadLength);
	void CloseSocket();
	void TryStartWrite();
	void WriteNextFrame();
	void ReportError();
	[[nodiscard]] bool IsDrained() const { return _writeQueue.empty() && !_writeInProgress; }
	void FinishDraining();

	tcp::socket _socket;
	std::string _ownerName;
	std::array<char, kFrameHeaderSize> _readHeader{};
	std::vector<char> _readPayload{};
	std::deque<std::shared_ptr<const std::string>> _writeQueue{};
	bool _writeInProgress{false};
	bool _writeEnabled{true};
	FrameHandler _onFrame{};
	ErrorHandler _onError{};
	DrainHandler _onDrained{};

	static constexpr std::size_t kMaxPendingFrames{1024u};
};
}//namespace network
