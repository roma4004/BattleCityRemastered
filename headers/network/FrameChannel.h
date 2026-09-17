#pragma once

#include "MessageFraming.h"
#include <array>
#include <boost/asio/ip/tcp.hpp>
#include <cstddef>
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

	//NOTE: read unsynchronised - a dead session may look open one frame longer, which lets its error
	//handler report the loss before the session is reaped
	[[nodiscard]] bool IsOpen() const { return _socket.is_open(); }
	[[nodiscard]] tcp::socket& Socket() { return _socket; }

private:
	void ReadHeader();
	void ReadPayload(std::uint32_t payloadLength);
	void CloseSocket();
	void TryStartWrite();
	void WriteNextFrame();
	void ReportError() const;
	[[nodiscard]] bool IsDrained() const { return _writeQueue.empty() && !_writeInProgress; }
	void FinishDraining();

	tcp::socket _socket;
	std::string _ownerName;
	std::array<char, kFrameHeaderSize> _readHeader{};
	std::vector<char> _readPayload{};
	std::deque<std::shared_ptr<const std::string>> _writeQueue{};
	bool _writeInProgress{false};
	//NOTE: bumped by every close - a handler issued for a link that is gone still reaches these fields
	std::uint32_t _linkEpoch{0};
	bool _writeEnabled{true};
	FrameHandler _onFrame{};
	ErrorHandler _onError{};
	DrainHandler _onDrained{};

	static constexpr std::size_t kMaxPendingFrames{1024u};
};
}//namespace network
