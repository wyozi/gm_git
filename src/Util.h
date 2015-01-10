#pragma once

#include "Module.h"

namespace Util {
	namespace Git {
		std::string ErrorToString(int error);
	}

	namespace GLua {
		void PrintMessage(lua_State* state, char* str);
	}
}