#include "Module.h"

int gmod_OpenRepo(lua_State* state) {
	const char* repo_path = LUA->CheckString(1);

	Repository** repo;
	Repository* repo_object;

	try {
		repo_object = new Repository(std::string(repo_path));
	} catch (GitError e) {
		LUA->PushBool(false);
		LUA->PushString(Util::Git::ErrorToString(e.error).c_str());
		return 2;
	}

	repo = (Repository**)LUA->NewUserdata(sizeof(Repository**));
	*repo = repo_object;

	LUA->CreateMetaTableType("GitRepository", 60263);

	LUA->SetMetaTable(-2);

	return 1;
}

int gmod_IsRepo(lua_State* state) {
	const char* repo_path = LUA->CheckString(1);
	LUA->PushBool(git_repository_open_ext(NULL, repo_path, GIT_REPOSITORY_OPEN_NO_SEARCH, NULL) == 0);

	return 1;
}

void CreateRepositoryMetatable(lua_State* state) {

	LUA->CreateMetaTableType("GitRepository", 60263);
		LUA->Push(-1);
		LUA->SetField(-2, "__index");
		
		LUA->PushCFunction(LuaBridge::Free);
		LUA->SetField(-2, "__gc");

		LUA->PushCFunction(LuaBridge::Fetch);
		LUA->SetField(-2, "Fetch");

		LUA->PushCFunction(LuaBridge::Push);
		LUA->SetField(-2, "Push");

		LUA->PushCFunction(LuaBridge::Commit);
		LUA->SetField(-2, "Commit");

		LUA->PushCFunction(LuaBridge::Pull);
		LUA->SetField(-2, "Pull");
		
		LUA->PushCFunction(LuaBridge::IndexEntries);
		LUA->SetField(-2, "IndexEntries");

		LUA->PushCFunction(LuaBridge::AddPathSpecToIndex);
		LUA->SetField(-2, "AddPathSpecToIndex");

		LUA->PushCFunction(LuaBridge::AddIndexEntry);
		LUA->SetField(-2, "AddIndexEntry");

		LUA->PushCFunction(LuaBridge::RemoveIndexEntry);
		LUA->SetField(-2, "RemoveIndexEntry");

		LUA->PushCFunction(LuaBridge::FileStatus);
		LUA->SetField(-2, "FileStatus");

		LUA->PushCFunction(LuaBridge::Status);
		LUA->SetField(-2, "Status");

		LUA->PushCFunction(LuaBridge::Log);
		LUA->SetField(-2, "Log");
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

GMOD_MODULE_OPEN() {
	git_threads_init();

	CreateRepositoryMetatable(state);
	CreateGModLibrary(state);

	return 0;
}

GMOD_MODULE_CLOSE() {
	return 0;
}