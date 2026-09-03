#include "network/FrameChannel.h"
#include "utils/Log.h"
#include <boost/asio/post.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <utility>

namespace network
{
namespace
{
//NOTE: both are the link ending the way it is meant to - a peer that closed, or our own cancel
void LogUnexpected(const std::string& ownerName, const boost::system::error_code& ec, const char* operation)
{
	if (ec != boost::asio::error::eof && ec != boost::asio::error::operation_aborted)
	{
		Log::Error(ownerName + " " + operation + ": " + ec.message());
	}
}
}//namespace

FrameChannel::FrameChannel(tcp::socket socket, std::string ownerName)
	: _socket(std::move(socket))
	, _ownerName(std::move(ownerName)) {}

void FrameChannel::SetHandlers(FrameHandler onFrame, ErrorHandler onError)
{
	_onFrame = std::move(onFrame);
	_onError = std::move(onError);
}

void FrameChannel::Close()
{
	_onFrame = nullptr;
	_onError = nullptr;
	CloseSocket();
	FinishDraining();
}

void FrameChannel::CloseForReconnect()
{
	_writeInProgress = false;
	//NOTE: the queue held frames for the link being replaced - stale input would reach the new one
	_writeQueue.clear();
	CloseSocket();
}

void FrameChannel::CloseAfterFlush(DrainHandler onClosed)
{
	auto self(shared_from_this());
	boost::asio::post(_socket.get_executor(), [this, self, handler = std::move(onClosed)]() mutable
	{
		_onFrame = nullptr;
		_onError = nullptr;
		_onDrained = std::move(handler);

		if (IsDrained() || !_writeEnabled || !_socket.is_open())
		{
			CloseSocket();
			FinishDraining();
			return;
		}

		TryStartWrite();
	});
}

void FrameChannel::FinishDraining()
{
	if (!_onDrained)
	{
		return;
	}

	const DrainHandler onDrained = std::move(_onDrained);
	_onDrained = nullptr;
	onDrained();
}

void FrameChannel::CloseSocket()
{
	if (!_socket.is_open())
	{
		return;
	}

	boost::system::error_code ec;
	std::ignore = _socket.cancel(ec);

	std::ignore = _socket.shutdown(tcp::socket::shutdown_both, ec);
	if (ec && ec != boost::asio::error::not_connected)
	{
		Log::Error(_ownerName + " socket shutdown: " + ec.message());
	}

	std::ignore = _socket.close(ec);
	if (ec)
	{
		Log::Error(_ownerName + " socket close: " + ec.message());
	}
}

void FrameChannel::ReportError()
{
	if (_onError)
	{
		_onError();
	}
}

void FrameChannel::StartReading() { ReadHeader(); }

void FrameChannel::ReadHeader()
{
	auto self(shared_from_this());
	boost::asio::async_read(_socket, boost::asio::buffer(_readHeader),
							[this, self](const boost::system::error_code& ec, std::size_t)
							{
								if (ec)
								{
									LogUnexpected(_ownerName, ec, "read");
									ReportError();
									return;
								}

								const std::uint32_t payloadLength = DecodeFrameHeader(_readHeader.data());
								if (payloadLength == 0u || payloadLength > kMaxFramePayloadSize)
								{
									Log::Info(_ownerName + ": bogus frame length "
											  + std::to_string(payloadLength)
											  + ", dropping connection");
									ReportError();
									return;
								}

								ReadPayload(payloadLength);
							});
}

void FrameChannel::ReadPayload(const std::uint32_t payloadLength)
{
	_readPayload.resize(payloadLength);

	auto self(shared_from_this());
	boost::asio::async_read(_socket, boost::asio::buffer(_readPayload),
							[this, self](const boost::system::error_code& ec, std::size_t)
							{
								if (ec)
								{
									LogUnexpected(_ownerName, ec, "read");
									ReportError();
									return;
								}

								if (_onFrame)
								{
									_onFrame(std::string(_readPayload.data(), _readPayload.size()));
								}

								ReadHeader();
							});
}

void FrameChannel::SetWriteEnabled(const bool enabled)
{
	_writeEnabled = enabled;
	if (enabled)
	{
		TryStartWrite();
	}
}

void FrameChannel::Send(std::shared_ptr<const std::string> frame)
{
	auto self(shared_from_this());
	boost::asio::post(_socket.get_executor(), [this, self, payload = std::move(frame)]() mutable
	{
		if (_writeQueue.size() >= kMaxPendingFrames)
		{
			_writeQueue.pop_front();
			Log::Info(_ownerName + ": pending queue full, dropped oldest frame");
		}

		_writeQueue.push_back(std::move(payload));

		TryStartWrite();
	});
}

void FrameChannel::TryStartWrite()
{
	if (_writeQueue.empty() || _writeInProgress || !_writeEnabled || !_socket.is_open())
	{
		return;
	}

	_writeInProgress = true;

	WriteNextFrame();
}

void FrameChannel::WriteNextFrame()
{
	const auto frame = _writeQueue.front();
	auto self(shared_from_this());
	boost::asio::async_write(_socket, boost::asio::buffer(*frame),
							 [this, self, frame](const boost::system::error_code& ec, std::size_t)
							 {
								 if (ec)
								 {
									 LogUnexpected(_ownerName, ec, "write");

									 _writeInProgress = false;

									 if (_onDrained)
									 {
										 CloseSocket();
										 FinishDraining();
										 return;
									 }

									 ReportError();
									 return;
								 }

								 _writeQueue.pop_front();

								 if (_writeQueue.empty())
								 {
									 _writeInProgress = false;

									 if (_onDrained)
									 {
										 CloseSocket();
										 FinishDraining();
									 }

									 return;
								 }

								 WriteNextFrame();
							 });
}
}//namespace network
