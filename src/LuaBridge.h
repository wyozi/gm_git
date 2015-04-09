#pragma once

#include "Module.h"
#include "Repository.h"

namespace LuaBridge {
	int Commit(lua_State* state);
	int Push(lua_State* state);
	int Pull(lua_State* state);

	int IndexEntries(lua_State* state);
	int AddPathSpecToIndex(lua_State* state);
	int AddIndexEntry(lua_State* state);
	int RemoveIndexEntry(lua_State* state);

	int FileStatus(lua_State* state);
	int Status(lua_State* state);

	int Log(lua_State* state);

	int DiffIndexToWorkdir(lua_State* state);
	int DiffHEADToIndex(lua_State* state);
	int DiffHEADToWorkdir(lua_State* state);

	int Free(lua_State* state);
}