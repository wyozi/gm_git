
//
// Called when you module is opened
//
#include "Module.h"

int gmod_OpenRepo(lua_State* state) {
	if (!LUA->IsType(1, GarrysMod::Lua::Type::STRING)) {
		LUA->PushNil();
		return 1;
	}

	const char* repo_path = LUA->GetString(1);

	Repository** repo;
	Repository* repo_object;

	repo_object = new Repository(std::string(repo_path));

	repo = (Repository**)LUA->NewUserdata(sizeof(Repository**));
	*repo = repo_object;

	LUA->CreateMetaTableType("GitRepository", 60263);

	LUA->SetMetaTable(-2);

	return 1;
}

int gmod_IsRepo(lua_State* state) {
	if (!LUA->IsType(1, GarrysMod::Lua::Type::STRING)) {
		LUA->PushBool(false);
		return 1;
	}

	const char* repo_path = LUA->GetString(1);
	LUA->PushBool(git_repository_open_ext(NULL, repo_path, GIT_REPOSITORY_OPEN_NO_SEARCH, NULL) == 0);

	return 1;
}

void CreateRepositoryMetatable(lua_State* state) {

	LUA->CreateMetaTableType("GitRepository", 60263);
		LUA->Push(-1);
		LUA->SetField(-2, "__index");

		LUA->PushCFunction(LuaBridge::Free);
		LUA->SetField(-2, "Free");

	LUA->Pop();
}

void CreateGModLibrary(lua_State* state) {
	LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		LUA->CreateTable();
			LUA->PushCFunction(gmod_OpenRepo);
			LUA->SetField(-2, "Open");

			LUA->PushCFunction(gmod_IsRepo);
			LUA->SetField(-2, "IsRepository");
		LUA->SetField(-2, "git");
	LUA->Pop();
}

GMOD_MODULE_OPEN()
{

	git_threads_init();

	CreateRepositoryMetatable(state);
	CreateGModLibrary(state);

	return 0;
}

GMOD_MODULE_CLOSE()
{
	return 0;
}