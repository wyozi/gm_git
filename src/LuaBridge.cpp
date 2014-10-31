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

	RepositoryStatus* status;
	try {
		status = repo->GetStatus();
	} catch (GitError e) {
		LUA->PushBool(false);
		LUA->PushString(Wyozi::Util::GitErrorToString(e.error).c_str());
		return 2;
	}
	
	LUA->CreateTable();

		LUA->PushString("Branch");
		if (!status->branch.empty())
			LUA->PushString(status->branch.c_str());
		else
			LUA->PushNil();
		LUA->SetTable(-3);
		
		LUA->PushString("IndexChanges");
		LUA->CreateTable();
		
		int i = 1;
		for (auto change = status->index_changes.begin(); change != status->index_changes.end(); ++change) {
			std::string old_path = (*change)->old_path;
			std::string new_path = (*change)->path;

			LUA->PushNumber(i);
			LUA->CreateTable();

				LUA->PushString("Status");
				LUA->PushString((*change)->type.c_str());
				LUA->SetTable(-3);

				LUA->PushString("Path");
				LUA->PushString(new_path.c_str());
				LUA->SetTable(-3);

				if (!old_path.empty() && old_path != new_path) {
					LUA->PushString("OldPath");
					LUA->PushString(old_path.c_str());
					LUA->SetTable(-3);
				}
			
			LUA->SetTable(-3);
			i++;
		}
		LUA->SetTable(-3);

		LUA->PushString("WorkDirChanges");
		LUA->CreateTable();
		
		i = 1;
		for (auto change = status->work_dir_changes.begin(); change != status->work_dir_changes.end(); ++change) {
			std::string old_path = (*change)->old_path;
			std::string new_path = (*change)->path;

			LUA->PushNumber(i);
			LUA->CreateTable();

				LUA->PushString("Status");
				LUA->PushString((*change)->type.c_str());
				LUA->SetTable(-3);

				LUA->PushString("Path");
				LUA->PushString(new_path.c_str());
				LUA->SetTable(-3);

				if (!old_path.empty() && old_path != new_path) {
					LUA->PushString("OldPath");
					LUA->PushString(old_path.c_str());
					LUA->SetTable(-3);
				}
			
			LUA->SetTable(-3);
			i++;
		}
		LUA->SetTable(-3);

	return 1;
}

int LuaBridge::Free(lua_State* state) {
	Repository* repo = fetchRepository(state);
	if (!repo)
		return 0;

	repo->Free();

	return 1;
}