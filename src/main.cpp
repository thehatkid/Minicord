#include <string>
#include <memory>
#include <iostream>
#include <signal.h>

#include "etc/utils.h"
#include "gateway/gateway.h"

using namespace minicord;

namespace
{
	std::shared_ptr<GatewayClient> client;

	void signal_handler(int /* signum */)
	{
		signal(SIGINT, signal_handler);

		client->stop();
	}
}

int main(int /* argc */, char **/* *argv[] */)
{
	signal(SIGINT, signal_handler);

	std::string token;
	if (!std::getenv("MINICORD_TOKEN")) {
		std::cerr << "Please set user token to \"MINICORD_TOKEN\" environment variable." << std::endl;
		return 1;
	} else {
		token = std::getenv("MINICORD_TOKEN");
	}

	// Initialize WinSock2 before running IXWebSocket on Windows
	if (!net_init()) {
		std::cerr << "Unable to initialize WinSock2!" << std::endl;
		return 1;
	}

	client = std::make_shared<GatewayClient>(token);

	// Run gateway client in blocking call
	client->run();

	// Uninitialize WinSock2
	net_quit();

	return 0;
}