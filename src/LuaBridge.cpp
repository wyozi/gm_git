#include "LuaBridge.h"

Repository * fetchRepository(lua_State* state) {
	if (!LUA->IsType(1, GarrysMod::Lua::Type::USERDATA)) {
		LUA->ThrowError("Use 'repo:Method()', not 'repo.Method()'");
		return NULL;
	}
	return *(Repository**)LUA->GetUserdata(1);
}

int LuaBridge::Status(lua_State* state) {
	Repository* repo = fetchRepository(state);
	if (!repo)
		return 0;

	RepositoryStatus* status = repo->GetStatus();

	return 1;
}

int LuaBridge::Free(lua_State* state) {
	Repository* repo = fetchRepository(state);
	if (!repo)
		return 0;

	repo->Free();

	return 1;
}