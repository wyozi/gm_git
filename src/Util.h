#pragma once

#include "Module.h"

namespace Wyozi {
	namespace Util {
		std::string GitErrorToString(int error);
		void PrintMessage(lua_State* state, char* str);
	}
};