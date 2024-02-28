#include "gateway.h"
#include "etc/utils.h"

#include <iostream>
#include <thread>
#include <chrono>

using namespace minicord;

GatewayClient::GatewayClient(std::string token)
	: gatewayUrl("wss://gateway.discord.gg"),
	  token(token),
	  connected(false),
	  autoreconnect(true),
	  ready(false),
	  resumable(false),
	  sequence(0)
{
}

void GatewayClient::run()
{
	std::cout << "[GatewayClient] Running..." << std::endl;

	autoreconnect = true;

	ws = make_unique<ix::WebSocket>();
	ws->disableAutomaticReconnection();

	// TODO: Customize User Agent depending on platforms
	ix::WebSocketHttpHeaders headers;
	headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/121.0.0.0 Safari/537.36";
	headers["Cache-Control"] = "no-cache";
	headers["Pragma"] = "no-cache";
	ws->setExtraHeaders(headers);

	ws->setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
		onMessage(const_cast<ix::WebSocketMessagePtr&>(msg));
	});

	std::string url;

	while (autoreconnect)
	{
		if (resumable && !resumeGatewayUrl.empty())
			url = resumeGatewayUrl;
		else
			url = gatewayUrl;

		url.append("/?v=10&encoding=json");

		ws->setUrl(url);

		// Connect to gateway and run in blocking mode
		ws->connect(10);
		ws->run();

		// Wait before next connection
		if (autoreconnect)
			std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	}
}

void GatewayClient::stop()
{
	std::cout << "[GatewayClient] Stopping..." << std::endl;

	autoreconnect = false;

	// Stop connections and close with 1000 code
	// to invalidate session and appear as Offline.
	ws->stop(1000, "");

	sequence = 0;
	resumable = false;
	resumeGatewayUrl.clear();
	sessionId.clear();
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

	if (op == GatewayOpcode::IDENTIFY || op == GatewayOpcode::RESUME) {
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

		if (!hb.ack) {
			// This can mean the connection is dead, so we will try to reconnect
			// and resume session (re-IDENTIFY if session got expired due inactivity)
			std::cout << "[GatewayClient] Last Heartbeat was not ACK'd, reconnecting..." << std::endl;
			ws->close(1006, "");
			break;
		}

		// Send HEARTBEAT opcode with last sequence number
		rapidjson::Value sequenceValue;
		if (sequence > 0)
			sequenceValue = sequence;

		sendOpcode(GatewayOpcode::HEARTBEAT, sequenceValue);

		hb.ack = false;
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
			ready = false;

			if (msg->closeInfo.remote)
				fprintf(stderr, "[GatewayClient] Gateway closed connection with %i: %s\n", msg->closeInfo.code, msg->closeInfo.reason.c_str());
			else
				fprintf(stderr, "[GatewayClient] Client closed connection with %i: %s\n", msg->closeInfo.code, msg->closeInfo.reason.c_str());

			// Stop Heartbeat thread
			hb.ack = false;
			hb.waiter.stop();
			if (hb.thread.joinable())
				hb.thread.join();

			// We cannot resume session if it experienced Gateway error closure code.
			// The session becomes invalid and next attempt to resume same session
			// will result repeat closure with same code.
			if (4000 <= msg->closeInfo.code && msg->closeInfo.code <= 4999) {
				resumable = false;
			}

			switch (msg->closeInfo.code)
			{
				case GatewayCloseCodes::NotAuthenticated:
				case GatewayCloseCodes::AuthenticationFailed:
				case GatewayCloseCodes::InvalidShard:
				case GatewayCloseCodes::ShardingRequired:
				case GatewayCloseCodes::InvalidAPIVersion:
				case GatewayCloseCodes::InvalidIntents:
				case GatewayCloseCodes::DisallowedIntents:
					// Turn off auto reconnection on these close codes
					autoreconnect = false;
					break;

				default:
					break;
			}

			break;

		case ix::WebSocketMessageType::Error:
			connected = false;
			ready = false;
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

	// For debugging purposes
	if (opcode == GatewayOpcode::DISPATCH)
		fprintf(stdout, "\x1B[94m[>] E: %s | S: %d\x1B[0m\n", event.c_str(), sequence);
	else
		fprintf(stdout, "\x1B[94m[>] OP: %d | S: %d | %s\x1B[0m\n", opcode, sequence, stringifyJSON(data).c_str());

	switch (opcode)
	{
		case GatewayOpcode::HELLO:
			// Set Heartbeat interval and start Heartbeating gateway
			hb.interval = data["heartbeat_interval"].GetUint();
			hb.ack = true;
			hb.waiter.start();
			hb.thread = std::thread(&GatewayClient::heartbeatHandler, this);

			if (resumable) {
				// Send RESUME payload to resume current session
				sendResume();
			} else {
				// Send IDENTIFY payload to make new session
				sendIdentify();
			}
			break;

		case GatewayOpcode::INVALID_SESSION:
			if (data.GetBool() && resumable) {
				std::cout << "[GatewayClient] Got INVALID_SESSION opcode and it's resumable, resuming..." << std::endl;

				sendResume();
			} else {
				std::cout << "[GatewayClient] Got INVALID_SESSION opcode, re-identifying..." << std::endl;

				sequence = 0;
				resumable = false;
				sessionId.clear();
				resumeGatewayUrl.clear();

				sendIdentify();
			}
			break;

		case GatewayOpcode::RECONNECT:
			std::cout << "[GatewayClient] Got RECONNECT opcode, reconnecting..." << std::endl;

			// Close connection with 1012 (Service is restarting) status code.
			// This is because it will invalidate session if we close WebSocket
			// with 1000 or 1001, but we supposed to resume current session.
			ws->close(1012, "");
			break;

		case GatewayOpcode::DISPATCH:
			handleEvent(event, data);
			break;

		case GatewayOpcode::HEARTBEAT_ACK:
			hb.ack = true;
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
		// Get Session ID and Resume Gateway URL for resuming session in next
		sessionId = data["session_id"].GetString();
		resumeGatewayUrl = data["resume_gateway_url"].GetString();
		resumable = true;

		// Get user data
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

		fprintf(stdout, "\x1B[93m[!] Session ID: %s\x1B[0m\n", sessionId.c_str());

		ready = true;
	} else if (event == "RESUMED") {
		fprintf(stdout, "\x1B[93m[!] Successfully resumed session %s\x1B[0m\n", sessionId.c_str());

		ready = true;
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

void GatewayClient::sendResume()
{
	rapidjson::Document resume(rapidjson::kObjectType);
	rapidjson::Document::AllocatorType& allocator = resume.GetAllocator();

	resume.AddMember("token", token, allocator);
	resume.AddMember("session_id", sessionId, allocator);
	resume.AddMember("seq", sequence, allocator);

	// Send RESUME payload
	sendOpcode(GatewayOpcode::RESUME, resume);
}