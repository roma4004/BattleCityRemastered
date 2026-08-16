#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace network
{
//NOTE: length prefix, not a delimiter - "\n\n" occurs by chance inside a binary ser20 archive
inline constexpr std::size_t kFrameHeaderSize = 4;

//NOTE: guards against a garbage header being read as a multi-gigabyte length
inline constexpr std::uint32_t kMaxFramePayloadSize = 4u * 1024u * 1024u;

inline std::uint32_t DecodeFrameHeader(const char* data)
{
	return (static_cast<std::uint32_t>(static_cast<unsigned char>(data[0])) << 24)
		   | (static_cast<std::uint32_t>(static_cast<unsigned char>(data[1])) << 16)
		   | (static_cast<std::uint32_t>(static_cast<unsigned char>(data[2])) << 8)
		   | static_cast<std::uint32_t>(static_cast<unsigned char>(data[3]));
}

inline std::string FrameMessage(const std::string& payload)
{
	const auto length = static_cast<std::uint32_t>(payload.size());
	const char header[kFrameHeaderSize]{
			static_cast<char>((length >> 24) & 0xFF),
			static_cast<char>((length >> 16) & 0xFF),
			static_cast<char>((length >> 8) & 0xFF),
			static_cast<char>(length & 0xFF),
	};

	std::string framed;
	framed.reserve(kFrameHeaderSize + payload.size());
	framed.append(header, kFrameHeaderSize);
	framed.append(payload);

	return framed;
}
}//namespace network
