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

//NOTE: length-prefixed framing over one socket, shared by Client and Session. Owns the socket, but
//hands it out for connect/shutdown - the owner still decides when the link opens and closes.
//All access happens on the socket's strand, hence no mutex on the queue.
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

	//NOTE: unlike Close(), lets the queue finish - a goodbye that never leaves is no goodbye.
	//onClosed runs exactly once, on the socket's strand.
	void CloseAfterFlush(DrainHandler onClosed);

	//NOTE: after a drop the failed write never completes, so nothing else would clear the flag;
	//the queue itself is kept, its head is re-sent on the next connection
	void ResetWriteState() { _writeInProgress = false; }

	//NOTE: frames queue up regardless, but nothing goes out until the owner says the link is usable -
	//Client only opens it once connected, Session is writable from the start
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
	//NOTE: one write in flight at a time - interleaved bytes desync length framing unrecoverably
	std::deque<std::shared_ptr<const std::string>> _writeQueue{};
	bool _writeInProgress{false};
	bool _writeEnabled{true};
	FrameHandler _onFrame{};
	ErrorHandler _onError{};
	//NOTE: while set, a read error must not reach the owner - we are the ones closing
	DrainHandler _onDrained{};

	//NOTE: bounds the queue on a dead link; the oldest frame is the stalest state, so it goes first
	static constexpr std::size_t MaxPendingFrames{1024u};
};
}//namespace network
