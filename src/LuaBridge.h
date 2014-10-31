#pragma once

#include "Module.h"
#include "Repository.h"

namespace LuaBridge {
	int Status(lua_State* state);
	int Free(lua_State* state);
}