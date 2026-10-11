#pragma once

#include <cstdint>

//NOTE: one server of ours per port at a time - a new one waits for the one winding down, which still listens and
//would close the port on the router after the new one opened it. Let go of however the server ends, a crash included
class PortClaim final
{
public:
	//NOTE: a port left to the OS is nobody's to wait for yet - it is taken once bound
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
	//NOTE: a HANDLE, held as what it is underneath so windows.h stays out of this header
	void* _mutex{nullptr};
	bool _isHeld{};
#else
	//NOTE: a flock goes with the descriptor - the kernel takes it back when the process dies
	int _lockFile{-1};
#endif
};
