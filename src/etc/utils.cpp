#include "utils.h"

#include <rapidjson/writer.h>
#include <ixwebsocket/IXNetSystem.h>

bool minicord::net_init()
{
	return ix::initNetSystem();
}

bool minicord::net_quit()
{
	return ix::uninitNetSystem();
}

std::string minicord::stringifyJSON(rapidjson::Value& json)
{
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	json.Accept(writer);
	return buffer.GetString();
}