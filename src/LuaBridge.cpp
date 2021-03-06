#include "LuaBridge.h"

Repository * fetchRepository(lua_State* state) {
	if (!LUA->IsType(1, GarrysMod::Lua::Type::USERDATA)) {
		LUA->ThrowError("Use 'repo:Method()', not 'repo.Method()'");
		return NULL;
	}
	return *(Repository**)LUA->GetUserdata(1);
}
#define CHECK_REPO(repo) if(!repo) { \
	LUA->ThrowError("Repository pointer is NULL!!"); \
	return 0; \
}

int pushErrorString(lua_State* state, const GitError& e) {
	LUA->PushBool(false);
	LUA->PushString(Util::Git::ErrorToString(e.error).c_str());
	return 2;
}

int LuaBridge::Commit(lua_State* state) {
	Repository* repo = fetchRepository(state);
	CHECK_REPO(repo)

	const char* commitmsg = LUA->IsType(2, GarrysMod::Lua::Type::STRING) ? LUA->GetString(2) : "";

	int commitcode;

	// Don't use CommitOptions* opts = new CommitOptions;
	// It crashes for some reason.
	// Probably some scrub reason I feel stupid about after I figure out what it is
	CommitOptions opts = {};

	if (LUA->IsType(2, GarrysMod::Lua::Type::STRING)) opts.commitmsg = LUA->GetString(2);
	if (LUA->IsType(3, GarrysMod::Lua::Type::STRING)) opts.committer_name = LUA->GetString(3);
	if (LUA->IsType(4, GarrysMod::Lua::Type::STRING)) opts.committer_email = LUA->GetString(4);

	try {
		commitcode = repo->Commit(&opts);
	}
	catch (GitError e) {
		return pushErrorString(state, e);
	}

	if (commitcode == GMGIT_COMMIT_NOCHANGES) {
		LUA->PushBool(false);
		LUA->PushString("no changes in index");
		return 2;
	}

	LUA->PushBool(true);
	return 1;
}

int LuaBridge::Push(lua_State* state) {
	Repository* repo = fetchRepository(state);
	CHECK_REPO(repo)

	const char* remotename = LUA->IsType(2, GarrysMod::Lua::Type::STRING) ? LUA->GetString(2) : "origin";
	try {
		repo->Push(remotename);
	} catch (GitError e) {
		return pushErrorString(state, e);
	}

	LUA->PushBool(true);
	return 1;
}
int LuaBridge::Pull(lua_State* state) {
	Repository* repo = fetchRepository(state);
	CHECK_REPO(repo)

	const char* remotename = LUA->IsType(2, GarrysMod::Lua::Type::STRING) ? LUA->GetString(2) : "origin";
	try {
		repo->Pull(remotename);
	} catch (GitError e) {
		return pushErrorString(state, e);
	}

	LUA->PushBool(true);
	return 1;
}

int LuaBridge::IndexEntries(lua_State* state) {
	Repository* repo = fetchRepository(state);
	CHECK_REPO(repo)
	
	LUA->CreateTable();

		std::vector<std::string> entry_paths;
		try {
			entry_paths = repo->GetIndexEntries();
		} catch (GitError e) {
			return pushErrorString(state, e);
		}

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
	CHECK_REPO(repo)
	
	LUA->CheckString(2);
	
	try {
		repo->AddPathSpecToIndex(std::string(LUA->GetString(2)));
	} catch (GitError e) {
		return pushErrorString(state, e);
	}
	
	LUA->PushBool(true);
	return 1;
}

int LuaBridge::AddIndexEntry(lua_State* state) {
	Repository* repo = fetchRepository(state);
	CHECK_REPO(repo)
	
	LUA->CheckString(2);
	
	try {
		repo->AddIndexEntry(std::string(LUA->GetString(2)));
	} catch (GitError e) {
		return pushErrorString(state, e);
	}
	
	LUA->PushBool(true);
	return 1;
}

int LuaBridge::RemoveIndexEntry(lua_State* state) {
	Repository* repo = fetchRepository(state);
	CHECK_REPO(repo)
	
	LUA->CheckString(2);
	
	try {
		repo->RemoveIndexEntry(std::string(LUA->GetString(2)));
	} catch (GitError e) {
		return pushErrorString(state, e);
	}
	
	LUA->PushBool(true);
	return 1;
}

int LuaBridge::FileStatus(lua_State* state) {
	Repository* repo = fetchRepository(state);
	CHECK_REPO(repo)

	const char* path = LUA->CheckString(2);
	
	unsigned int status;
	try {
		status = repo->GetFileStatus(std::string(path));
	} catch (GitError e) {
		return pushErrorString(state, e);
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
	CHECK_REPO(repo)

	RepositoryStatus* status;
	try {
		status = repo->GetStatus();
	} catch (GitError e) {
		return pushErrorString(state, e);
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

		LUA->PushString("WorkdirChanges");
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

int LuaBridge::Log(lua_State* state) {
	Repository* repo = fetchRepository(state);
	CHECK_REPO(repo)

	RepositoryLog* log;
	try {
		log = repo->GetLog();
	} catch (GitError e) {
		return pushErrorString(state, e);
	}
	
	LUA->CreateTable();

	int i = 1;
	for (auto entry = log->log_entries.begin(); entry != log->log_entries.end(); ++entry) {
		LUA->PushNumber(i);
		LUA->CreateTable();

			LUA->PushString("Ref");
			LUA->PushString((*entry)->ref.c_str());
			LUA->SetTable(-3);

			LUA->PushString("Message");
			LUA->PushString((*entry)->commitmsg.c_str());
			LUA->SetTable(-3);

			LUA->PushString("Committer");
			LUA->PushString((*entry)->committer.c_str());
			LUA->SetTable(-3);

			LUA->PushString("Author");
			LUA->PushString((*entry)->author.c_str());
			LUA->SetTable(-3);
			
		LUA->SetTable(-3);
		i++;
	}

	return 1;
}

int LuaBridge::DiffIndexToWorkdir(lua_State* state) {
	Repository* repo = fetchRepository(state);
	CHECK_REPO(repo)
	
	try {
		std::string diff = repo->DiffIndexToWorkdir();

		LUA->PushString(diff.c_str());
		return 1;
	} catch (GitError e) {
		return pushErrorString(state, e);
	}
}

int LuaBridge::DiffHEADToIndex(lua_State* state) {
	Repository* repo = fetchRepository(state);
	CHECK_REPO(repo)
	
	try {
		std::string diff = repo->DiffHEADToIndex();

		LUA->PushString(diff.c_str());
		return 1;
	} catch (GitError e) {
		return pushErrorString(state, e);
	}
}


int LuaBridge::DiffHEADToWorkdir(lua_State* state) {
	Repository* repo = fetchRepository(state);
	CHECK_REPO(repo)
	
	try {
		std::string diff = repo->DiffHEADToWorkdir();

		LUA->PushString(diff.c_str());
		return 1;
	} catch (GitError e) {
		return pushErrorString(state, e);
	}
}


int LuaBridge::Free(lua_State* state) {
	Repository* repo = fetchRepository(state);
	CHECK_REPO(repo)

	repo->Free();
	repo = NULL;
	
	LUA->PushBool(true);
	return 1;
}