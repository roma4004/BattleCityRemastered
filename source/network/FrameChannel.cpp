#include "network/FrameChannel.h"
#include "utils/NetworkLogger.h"
#include <boost/asio/post.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <utility>

namespace network
{
FrameChannel::FrameChannel(tcp::socket socket, std::string ownerName)
	: _socket(std::move(socket))
	, _ownerName(std::move(ownerName))
{
}

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
	FinishDraining();//NOTE: a Close mid-drain still owes the waiter its callback, or teardown hangs
}

void FrameChannel::CloseAfterFlush(DrainHandler onClosed)
{
	//NOTE: posted, not run here - Send() posts too, so the goodbye is still on its way to the queue;
	//inline would find it empty and close before writing it
	auto self(shared_from_this());
	boost::asio::post(_socket.get_executor(), [this, self, onClosed = std::move(onClosed)]() mutable
	{
		//NOTE: dropped, or the owner would read our own shutdown as a dropped link
		_onFrame = nullptr;
		_onError = nullptr;
		_onDrained = std::move(onClosed);

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

	//NOTE: moved out before the call - the handler may destroy the owner that keeps us alive
	const DrainHandler onDrained = std::move(_onDrained);
	_onDrained = nullptr;
	onDrained();
}

//NOTE: cancel before shutdown/close - an abrupt close reads as a reset (WSAECONNRESET) on the peer
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
		NetworkLogger::WriteError(_ownerName + " socket shutdown: " + ec.message());
	}

	std::ignore = _socket.close(ec);
	if (ec)
	{
		NetworkLogger::WriteError(_ownerName + " socket close: " + ec.message());
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
									if (ec != boost::asio::error::eof
										&& ec != boost::asio::error::operation_aborted)
									{
										NetworkLogger::WriteError(_ownerName + " read: " + ec.message());
									}

									ReportError();
									return;
								}

								const std::uint32_t payloadLength = DecodeFrameHeader(_readHeader.data());
								if (payloadLength == 0u || payloadLength > kMaxFramePayloadSize)
								{
									NetworkLogger::WriteLog(_ownerName + ": bogus frame length "
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
									if (ec != boost::asio::error::eof
										&& ec != boost::asio::error::operation_aborted)
									{
										NetworkLogger::WriteError(_ownerName + " read: " + ec.message());
									}

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
	boost::asio::post(_socket.get_executor(), [this, self, frame = std::move(frame)]() mutable
	{
		if (_writeQueue.size() >= MaxPendingFrames)
		{
			_writeQueue.pop_front();
			NetworkLogger::WriteLog(_ownerName + ": pending queue full, dropped oldest frame");
		}

		_writeQueue.push_back(std::move(frame));

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
	const auto frame = _writeQueue.front();//NOTE: shared - the queue stays free while the write is in flight

	auto self(shared_from_this());
	boost::asio::async_write(_socket, boost::asio::buffer(*frame),
							 [this, self, frame](const boost::system::error_code& ec, std::size_t)
							 {
								 if (ec)
								 {
									 if (ec != boost::asio::error::eof
										 && ec != boost::asio::error::operation_aborted)
									 {
										NetworkLogger::WriteError(_ownerName + " write: " + ec.message());
									 }

									 //NOTE: undelivered frame stays at the head, re-sent whole next time
									 _writeInProgress = false;

									 //NOTE: undeliverable goodbye - waiting on a dead link would stall
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
