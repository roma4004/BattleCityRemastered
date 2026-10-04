/* Written for the .sln build: CMake generates the same file from ThirdParty/miniupnp/miniupnpc/miniupnpcstrings.h.cmake,
 * the upstream VisualC project from a VBScript that asks WMI for the OS version. The version is bumped along with the
 * submodule. */
#ifndef MINIUPNPCSTRINGS_H_INCLUDED
#define MINIUPNPCSTRINGS_H_INCLUDED

#define OS_STRING "Windows"
#define MINIUPNPC_VERSION_STRING "2.3.3"

/* according to "UPnP Device Architecture 1.1" */
#define UPNP_VERSION_MAJOR 1
#define UPNP_VERSION_MINOR 1
#define UPNP_VERSION_STRING "UPnP/1.1"

#endif
