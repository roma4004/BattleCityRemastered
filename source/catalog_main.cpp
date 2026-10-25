#include "network/Endpoints.h"
#include "network/ServerCatalog.h"
#include "utils/Log.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/system/error_code.hpp>
#include <csignal>
#include <memory>
#include <string>

//NOTE: the standalone catalog on every interface, IPv4 and IPv6
int main()
{
	Log::SetConsole(true);
	Log::SetFile(true, "catalog_log.txt");
	Log::SetLevel(Log::Level::Normal);

	boost::asio::io_context ioContext;
	auto catalog{std::make_unique<network::ServerCatalog>(
			ioContext, network::ServerAddress{.host = "::", .port = network::kCatalogPort})};
	if (catalog->BoundPort() == network::kAnyFreePort)
	{
		Log::Info("catalog: trying IPv4 alone");
		catalog = std::make_unique<network::ServerCatalog>(
				ioContext, network::ServerAddress{.host = "0.0.0.0", .port = network::kCatalogPort});
	}

	//NOTE: the catalog already said why
	if (catalog->BoundPort() == network::kAnyFreePort)
	{
		return 1;
	}

	boost::asio::signal_set signals{ioContext, SIGINT, SIGTERM};
	signals.async_wait([&catalog](const boost::system::error_code&, int) { catalog->Shutdown(); });

	Log::Info("catalog listening on port " + std::to_string(catalog->BoundPort()) + ", ctrl+c to stop");
	ioContext.run();
	Log::Info("catalog stopped");

	return 0;
}
