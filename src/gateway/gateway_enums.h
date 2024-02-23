#ifndef GATEWAY_ENUMS_H
#define GATEWAY_ENUMS_H

namespace minicord
{
	enum GatewayOpcode
	{
		DISPATCH = 0,
		HEARTBEAT = 1,
		IDENTIFY = 2,
		PRESENCE_UPDATE = 3,
		VOICE_STATE_UPDATE = 4,
		VOICE_SERVER_PING = 5,
		RESUME = 6,
		RECONNECT = 7,
		REQUEST_GUILD_MEMBERS = 8,
		INVALID_SESSION = 9,
		HELLO = 10,
		HEARTBEAT_ACK = 11,
		GUILD_SYNC = 12, // Deprecated
		CALL_CONNECT = 13,
		GUILD_SUBSCRIPTIONS = 14,
		LOBBY_CONNECT = 15,
		LOBBY_DISCONNECT = 16,
		LOBBY_VOICE_STATES_UPDATE = 17,
		STREAM_CREATE = 18,
		STREAM_DELETE = 19,
		STREAM_WATCH = 20,
		STREAM_PING = 21,
		STREAM_SET_PAUSED = 22,
		LFG_SUBSCRIPTIONS = 23, // Deprecated
		REQUEST_GUILD_APPLICATION_COMMANDS = 24, // Deprecated
		EMBEDDED_ACTIVITY_LAUNCH = 25,
		EMBEDDED_ACTIVITY_CLOSE = 26,
		EMBEDDED_ACTIVITY_UPDATE = 27,
		REQUEST_FORUM_UNREADS = 28,
		REMOTE_COMMAND = 29,
		GET_DELETED_ENTITY_IDS_NOT_MATCHING_HASH = 30,
		REQUEST_SOUNDBOARD_SOUNDS = 31,
		SPEED_TEST_CREATE = 32,
		SPEED_TEST_DELETE = 33,
		REQUEST_LAST_MESSAGES = 34,
		SEARCH_RECENT_MEMBERS = 35,
		REQUEST_CHANNEL_STATUES = 36,
		GUILD_SUBSCRIPTIONS_BULK = 37
	};

	enum GatewayCloseCodes
	{
		UnknownError = 4000,
		UnknownOpcode = 4001,
		DecodeError = 4002,
		NotAuthenticated = 4003,
		AuthenticationFailed = 4004,
		AlreadyAuthenticated = 4005,
		InvalidSequence = 4007,
		RateLimited = 4008,
		SessionTimedOut = 4009,
		InvalidShard = 4010,
		ShardingRequired = 4011,
		InvalidAPIVersion = 4012,
		InvalidIntents = 4013,
		DisallowedIntents = 4014
	};

	enum GatewayCapabilities
	{
		LAZY_USER_NOTES = 1 << 0,
		NO_AFFINE_USER_IDS = 1 << 1,
		VERSIONED_READ_STATES = 1 << 2,
		VERSIONED_USER_GUILD_SETTINGS = 1 << 3,
		DEDUPE_USER_OBJECTS = 1 << 4,
		PRIORITIZED_READY_PAYLOAD = 1 << 5,
		MULTIPLE_GUILD_EXPERIMENT_POPULATIONS = 1 << 6,
		NON_CHANNEL_READ_STATES = 1 << 7,
		AUTH_TOKEN_REFRESH = 1 << 8,
		USER_SETTINGS_PROTO = 1 << 9,
		CLIENT_STATE_V2 = 1 << 10,
		PASSIVE_GUILD_UPDATE = 1 << 11,
		UNKNOWN_12 = 1 << 12,
		UNKNOWN_13 = 1 << 13
	};

	inline GatewayCapabilities operator|(GatewayCapabilities a, GatewayCapabilities b)
	{
		return static_cast<GatewayCapabilities>(static_cast<int>(a) | static_cast<int>(b));
	}
}

#endif // GATEWAY_ENUMS_H