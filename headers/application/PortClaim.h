#pragma once

#include <cstdint>

//NOTE: one server per port - a new one waits until the old one closes its router mapping; released even on a crash
class PortClaim final
{
public:
	//NOTE: port 0 claims nothing yet - Take claims the port the OS bound
	explicit PortClaim(std::uint16_t port);

	~PortClaim();

	PortClaim(const PortClaim&) = delete;
	PortClaim& operator=(const PortClaim&) = delete;
	PortClaim(PortClaim&&) = delete;
	PortClaim& operator=(PortClaim&&) = delete;

	//NOTE: the port the OS gave may be one a server winding down has just let go of; nothing once one is claimed
	void Take(std::uint16_t port);

private:
#ifdef _WIN32
	//NOTE: a HANDLE
	void* _mutex{nullptr};
	bool _isHeld{};
#else
	//NOTE: a flock goes with the descriptor - the kernel takes it back when the process dies
	int _lockFile{-1};
#endif
};
