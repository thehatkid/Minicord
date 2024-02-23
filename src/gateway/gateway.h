#ifndef GATEWAY_H
#define GATEWAY_H

#include "gateway_enums.h"
#include "etc/utils.h"
#include "objects/user.h"

#include <rapidjson/document.h>
#include <ixwebsocket/IXWebSocket.h>

namespace minicord
{
	class GatewayClient
	{
	public:
		GatewayClient(std::string token);

		void run();
		void stop();

		void send(std::string text);
		void sendOpcode(GatewayOpcode op, rapidjson::Value& data);

	private:
		void onMessage(const ix::WebSocketMessagePtr& msg);
		void parsePayload(std::string message);
		void handleEvent(std::string event, rapidjson::Document& data);

		void heartbeatHandler();

		void sendIdentify();
		void sendResume();

		std::unique_ptr<ix::WebSocket> ws;

		std::string gatewayUrl;
		std::string resumeGatewayUrl;
		std::string token;
		std::string sessionId;

		// 16381 is current default value
		uint32_t capabilities = (
			GatewayCapabilities::LAZY_USER_NOTES |
			GatewayCapabilities::VERSIONED_READ_STATES |
			GatewayCapabilities::VERSIONED_USER_GUILD_SETTINGS |
			GatewayCapabilities::DEDUPE_USER_OBJECTS |
			GatewayCapabilities::PRIORITIZED_READY_PAYLOAD |
			GatewayCapabilities::MULTIPLE_GUILD_EXPERIMENT_POPULATIONS |
			GatewayCapabilities::NON_CHANNEL_READ_STATES |
			GatewayCapabilities::AUTH_TOKEN_REFRESH |
			GatewayCapabilities::USER_SETTINGS_PROTO |
			GatewayCapabilities::CLIENT_STATE_V2 |
			GatewayCapabilities::PASSIVE_GUILD_UPDATE |
			GatewayCapabilities::UNKNOWN_12 |
			GatewayCapabilities::UNKNOWN_13
		);

		bool connected;
		bool autoreconnect;
		bool ready;
		bool resumable;
		uint32_t sequence;

		struct HeartbeatData
		{
			uint32_t interval;
			std::thread thread;
			Waiter waiter;
		} hb;

		User me;
	};
}

#endif // GATEWAY_H