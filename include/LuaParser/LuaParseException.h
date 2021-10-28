#pragma once

#include <exception>
#include <string_view>

class LuaParserException : std::exception
{
public:
	// ����gcc
	LuaParserException(const char* message)
		: exception(message)
	{
	}
};
