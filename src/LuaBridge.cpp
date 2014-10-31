#include "LuaBridge.h"

Repository * fetchRepository(lua_State* state) {
	if (!LUA->IsType(1, GarrysMod::Lua::Type::USERDATA)) {
		LUA->ThrowError("Use 'repo:Method()', not 'repo.Method()'");
		return NULL;
	}
	return *(Repository**)LUA->GetUserdata(1);
}

int LuaBridge::Fetch(lua_State* state) {
	Repository* repo = fetchRepository(state);
	if (!repo)
		return 0;

	const char* remotename = LUA->IsType(2, GarrysMod::Lua::Type::STRING) ? LUA->GetString(2) : "origin";
	repo->Fetch(remotename);

	LUA->PushBool(true);
	return 1;
}

int LuaBridge::Push(lua_State* state) {
	Repository* repo = fetchRepository(state);
	if (!repo)
		return 0;

	const char* remotename = LUA->IsType(2, GarrysMod::Lua::Type::STRING) ? LUA->GetString(2) : "origin";
	repo->Push(remotename);

	LUA->PushBool(true);
	return 1;
}

int LuaBridge::Commit(lua_State* state) {
	Repository* repo = fetchRepository(state);
	if (!repo)
		return 0;

	const char* commitmsg = LUA->IsType(2, GarrysMod::Lua::Type::STRING) ? LUA->GetString(2) : "";
	repo->Commit(commitmsg);

	LUA->PushBool(true);
	return 1;
}

int LuaBridge::IndexEntries(lua_State* state) {
	Repository* repo = fetchRepository(state);
	if (!repo)
		return 0;
	
	LUA->CreateTable();

		std::vector<std::string> entry_paths = repo->GetIndexEntries();
		int i = 1;
		for (auto entry = entry_paths.begin(); entry != entry_paths.end(); ++entry) {
			LUA->PushNumber(i);
			LUA->PushString(entry->c_str());
			LUA->SetTable(-3);

			i++;
		}

	return 1;
}

int LuaBridge::AddPathSpecToIndex(lua_State* state) {
	Repository* repo = fetchRepository(state);
	if (!repo)
		return 0;
	
	if (!LUA->IsType(2, GarrysMod::Lua::Type::STRING)) {
		LUA->ThrowError("AddPathSpecToIndex requires a string argument");
		return 0;
	}

	repo->AddPathSpecToIndex(std::string(LUA->GetString(2)));

	return 0;
}

int LuaBridge::AddToIndex(lua_State* state) {
	Repository* repo = fetchRepository(state);
	if (!repo)
		return 0;
	
	if (!LUA->IsType(2, GarrysMod::Lua::Type::STRING)) {
		LUA->ThrowError("AddToIndex requires a string argument");
		return 0;
	}

	repo->AddToIndex(std::string(LUA->GetString(2)));

	return 0;
}

int LuaBridge::RemoveFromIndex(lua_State* state) {
	Repository* repo = fetchRepository(state);
	if (!repo)
		return 0;
	
	if (!LUA->IsType(2, GarrysMod::Lua::Type::STRING)) {
		LUA->ThrowError("RemoveFromIndex requires a string argument");
		return 0;
	}

	repo->RemoveFromIndex(std::string(LUA->GetString(2)));

	return 0;
}

int LuaBridge::FileStatus(lua_State* state) {
	Repository* repo = fetchRepository(state);
	if (!repo)
		return 0;
	
	if (!LUA->IsType(2, GarrysMod::Lua::Type::STRING)) {
		LUA->ThrowError("FileStatus requires a string argument");
		return 0;
	}

	const char* path = LUA->GetString(2);
	
	unsigned int status;
	try {
		status = repo->GetFileStatus(std::string(path));
	} catch (GitError e) {
		LUA->PushBool(false);
		LUA->PushString(Wyozi::Util::GitErrorToString(e.error).c_str());
		return 2;
	}
	
	std::string type;
	if (status & GIT_STATUS_INDEX_NEW || status & GIT_STATUS_WT_NEW)
		type = ChangeType::enum_strings[ChangeType::NEW];
	if (status & GIT_STATUS_INDEX_MODIFIED || status & GIT_STATUS_WT_MODIFIED)
		type = ChangeType::enum_strings[ChangeType::MODIFIED];
	if (status & GIT_STATUS_INDEX_DELETED || status & GIT_STATUS_WT_DELETED)
		type = ChangeType::enum_strings[ChangeType::DELETED];
	if (status & GIT_STATUS_INDEX_RENAMED || status & GIT_STATUS_WT_RENAMED)
		type = ChangeType::enum_strings[ChangeType::RENAMED];
	if (status & GIT_STATUS_INDEX_TYPECHANGE || status & GIT_STATUS_WT_TYPECHANGE)
		type = ChangeType::enum_strings[ChangeType::TYPECHANGE];

	if (type.empty()) {
		LUA->PushNil();
		return 1;
	}

	LUA->PushString(type.c_str());
	return 1;
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