#pragma once

#include "Module.h"
#include "Repository.h"

namespace LuaBridge {
	int Fetch(lua_State* state);
	int Push(lua_State* state);
	
	int Commit(lua_State* state);

	int IndexEntries(lua_State* state);
	int AddPathSpecToIndex(lua_State* state);
	int AddToIndex(lua_State* state);
	int RemoveFromIndex(lua_State* state);

	int FileStatus(lua_State* state);
	int Status(lua_State* state);

	int Free(lua_State* state);
}