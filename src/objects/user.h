#ifndef OBJECTS_USER_H
#define OBJECTS_USER_H

#include <string>

namespace minicord
{
	struct User
	{
		std::string id;
		std::string username;
		std::string discriminator;
		std::string global_name;
		std::string pronouns;
		std::string avatar;
		std::string banner;
	};
}

#endif // OBJECTS_USER_H