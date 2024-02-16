#include "gateway.h"
#include "etc/utils.h"

#include <iostream>

using namespace minicord;

GatewayClient::GatewayClient(std::string token)
	: endpoint("wss://gateway.discord.gg/?v=10&encoding=json"),
	  token(token),
	  connected(false),
	  sequence(0)
{
}

void GatewayClient::run()
{
	std::cout << "[GatewayClient] Running..." << std::endl;

	ws = make_unique<ix::WebSocket>();
	ws->setUrl(endpoint);
	ws->disableAutomaticReconnection();

	ws->setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
		onMessage(const_cast<ix::WebSocketMessagePtr&>(msg));
	});

	// Connect to gateway and run in blocking mode
	ws->connect(10);
	ws->run();
}

void GatewayClient::stop()
{
	std::cout << "[GatewayClient] Stopping..." << std::endl;

	ws->stop(ix::WebSocketCloseConstants::kNormalClosureCode);
}

void GatewayClient::send(std::string text)
{
	if (!connected)
		return;

	ws->sendText(text);
}

void GatewayClient::sendOpcode(GatewayOpcode op, rapidjson::Value& data)
{
	if (!connected)
		return;

	rapidjson::Document payload(rapidjson::kObjectType);
	payload.AddMember("op", op, payload.GetAllocator());
	payload.AddMember("d", data, payload.GetAllocator());

	std::string msg = stringifyJSON(payload);

	if (op == GatewayOpcode::IDENTIFY) {
		// Exclude outputing user token in payload
		fprintf(stdout, "\x1B[92m[<] OP: %d\x1B[0m\n", op);
	} else {
		std::string dataStr = stringifyJSON(payload["d"]);
		fprintf(stdout, "\x1B[92m[<] OP: %d | %s\x1B[0m\n", op, dataStr.c_str());
	}

	send(msg);
}

void GatewayClient::heartbeatHandler()
{
	while (connected)
	{
		if (!hb.waiter.wait_for(std::chrono::milliseconds(hb.interval)))
			break;

		// Send HEARTBEAT opcode with last sequence number
		rapidjson::Value sequenceValue;
		if (sequence > 0)
			sequenceValue = sequence;

		sendOpcode(GatewayOpcode::HEARTBEAT, sequenceValue);
	}
}

void GatewayClient::onMessage(const ix::WebSocketMessagePtr& msg)
{
	switch (msg->type)
	{
		case ix::WebSocketMessageType::Open:
			connected = true;
			fprintf(stdout, "[GatewayClient] WebSocket is Open\n");
			break;

		case ix::WebSocketMessageType::Close:
			connected = false;
			fprintf(stderr, "[GatewayClient] WebSocket is Closed (%i: %s)\n", msg->closeInfo.code, msg->closeInfo.reason.c_str());

			// Stop Heartbeat thread
			hb.waiter.stop();
			if (hb.thread.joinable())
				hb.thread.join();

			break;

		case ix::WebSocketMessageType::Error:
			connected = false;
			fprintf(stderr, "[GatewayClient] WebSocket error: %s\n", msg->errorInfo.reason.c_str());
			break;

		case ix::WebSocketMessageType::Message:
			// TODO: Support zlib-stream compression
			parsePayload(msg->str);
			break;

		default:
			break;
	}
}

void GatewayClient::parsePayload(std::string message)
{
	rapidjson::Document payload;
	payload.Parse(message);

	if (payload.HasParseError()) {
		fprintf(stderr, "Failed to parse JSON payload (%d)\n", payload.GetParseError());
		return;
	}

	// Get opcode of payload
	uint16_t opcode = payload["op"].GetUint();

	// Update last payload sequence number
	if (!payload["s"].IsNull())
		sequence = payload["s"].GetUint();

	// Get payload dispatch event name
	std::string event = "";
	if (!payload["t"].IsNull())
		event = payload["t"].GetString();

	// Get payload data
	rapidjson::Document data(rapidjson::kObjectType);
	data.CopyFrom(payload["d"], data.GetAllocator());

	if (opcode == GatewayOpcode::DISPATCH)
		fprintf(stdout, "\x1B[94m[>] E: %s | S: %d\x1B[0m\n", event.c_str(), sequence);
	else
		fprintf(stdout, "\x1B[94m[>] OP: %d | S: %d | %s\x1B[0m\n", opcode, sequence, stringifyJSON(data).c_str());

	switch (opcode)
	{
		case GatewayOpcode::HELLO:
			// Set Heartbeat interval and start Heartbeating gateway
			hb.interval = data["heartbeat_interval"].GetUint();
			hb.waiter.start();
			hb.thread = std::thread(&GatewayClient::heartbeatHandler, this);

			// Send IDENTIFY payload
			sendIdentify();
			break;

		case GatewayOpcode::DISPATCH:
			handleEvent(event, data);
			break;

		default:
			break;
	}
}

void GatewayClient::handleEvent(std::string event, rapidjson::Document& data)
{
	// TODO: Implement more events handling and objects
	// TODO: Move to signals and slots

	if (event == "READY") {
		rapidjson::Document user(rapidjson::kObjectType);
		user.CopyFrom(data["user"], user.GetAllocator());

		me.id = user["id"].GetString();
		me.username = user["username"].GetString();
		me.discriminator = user["discriminator"].GetString();

		if (!user["global_name"].IsNull())
			me.global_name = user["global_name"].GetString();

		if (!user["pronouns"].IsNull())
			me.pronouns = user["pronouns"].GetString();

		if (!user["avatar"].IsNull())
			me.avatar = user["avatar"].GetString();

		if (!user["banner"].IsNull())
			me.banner = user["banner"].GetString();

		// Print welcome text
		if (me.discriminator != "0")
			fprintf(stdout, "\x1B[93m[!] Welcome, %s#%s! (ID: %s)\x1B[0m\n", me.username.c_str(), me.discriminator.c_str(), me.id.c_str());
		else
			fprintf(stdout, "\x1B[93m[!] Welcome, %s! (ID: %s)\x1B[0m\n", me.username.c_str(), me.id.c_str());
	}
}

void GatewayClient::sendIdentify()
{
	rapidjson::Document identity(rapidjson::kObjectType);
	rapidjson::Document::AllocatorType& allocator = identity.GetAllocator();

	identity.AddMember("token", token, allocator);
	identity.AddMember("capabilities", capabilities, allocator);

	// "Discord Client" if need to have Desktop status.
	// "Discord Android" or "Discord iOS" if need to have Mobile status/indicator.
	// Any "browser" name if need to have Web status.
	std::string browserName = "Discord Client";

	std::string browserUA = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/121.0.0.0 Safari/537.36";
	std::string browserVersion = "121.0.0.0";

	// TODO: Get latest client_build_number for release_channel on application start
	int buildNumber = 266159;

	// User session properties
	// TODO: Customize OS, Browser, User Agent, Locale fields depending on platforms
	rapidjson::Value properties(rapidjson::kObjectType);
	properties.AddMember("os", "Windows", allocator);
	properties.AddMember("os_version", "10.0.19045", allocator);
	properties.AddMember("os_arch", "x64", allocator);
	properties.AddMember("device", "", allocator);
	properties.AddMember("system_locale", "en", allocator);
	properties.AddMember("browser", browserName, allocator);
	properties.AddMember("browser_user_agent", browserUA, allocator);
	properties.AddMember("browser_version", browserVersion, allocator);
	properties.AddMember("release_channel", "stable", allocator);
	properties.AddMember("client_build_number", buildNumber, allocator);
	properties.AddMember("client_event_source", rapidjson::kNullType, allocator);

	identity.AddMember("properties", properties, allocator);

	// User session presence
	rapidjson::Value presence(rapidjson::kObjectType);
	presence.AddMember("status", "unknown", allocator);
	presence.AddMember("since", 0, allocator);
	presence.AddMember("activities", rapidjson::kArrayType, allocator);
	presence.AddMember("afk", false, allocator);
	presence.AddMember("broadcast", rapidjson::kNullType, allocator);

	identity.AddMember("presence", presence, allocator);

	identity.AddMember("compress", false, allocator);

	// User session client state
	rapidjson::Value state(rapidjson::kObjectType);
	state.AddMember("guild_versions", rapidjson::kObjectType, allocator);
	state.AddMember("highest_last_message_id", 0, allocator);
	state.AddMember("read_state_version", 0, allocator);
	state.AddMember("user_guild_settings_version", -1, allocator);
	state.AddMember("user_settings_version", rapidjson::kNullType, allocator);
	state.AddMember("private_channels_version", 0, allocator);
	state.AddMember("api_code_version", 0, allocator);
	state.AddMember("initial_guild_id", rapidjson::kNullType, allocator);

	identity.AddMember("client_state", state, allocator);

	// Send IDENTIFY payload
	sendOpcode(GatewayOpcode::IDENTIFY, identity);
}