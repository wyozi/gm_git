

// This should probably be a function..
#define CHECK_GIT_ERR(errid) if (errid < 0) {\
							LUA->PushBool(false);\
							const git_error *e = giterr_last();\
							std::ostringstream stringStream;\
							stringStream << "Error: " << errid << "/" << e->klass << ": " << e->message;\
							LUA->PushString(stringStream.str().c_str());\
							return 2;\
						 }



using namespace GarrysMod::Lua;

static int metatable_gitrepo;

bool IsGitError(int errid, lua_State* state) {
	if (errid < 0) {
		LUA->PushBool(false);

		const git_error *e = giterr_last();

		std::ostringstream stringStream;

		stringStream << "Error: " << errid << "/" << e->klass << ": " << e->message;

		LUA->PushString(stringStream.str().c_str());
		return true;
	}
	return false;
}


void GMod_DebugMsg(const char* msg, lua_State* state) {
	LUA->PushSpecial( SPECIAL_GLOB ); // +1
		LUA->GetField( -1, "MsgN" ); // +1

		LUA->PushString(msg); // +1
			LUA->Call(1, 0);
}

int Git_OpenRepo( lua_State* state )
{
	if ( LUA->IsType( 1, Type::STRING ) )
	{
		const char* path = LUA->GetString( 1 );
		git_repository *repo;

		int error = git_repository_open( &repo, path );
		CHECK_GIT_ERR(error);

		GarrysMod::Lua::UserData* ud = ( GarrysMod::Lua::UserData* )LUA->NewUserdata( sizeof( GarrysMod::Lua::UserData ) );
		ud->data = repo;
		ud->type = GarrysMod::Lua::Type::USERDATA; // ???
		LUA->ReferencePush( metatable_gitrepo );
		LUA->SetMetaTable( -2 );

		return 1;
	}
	
	LUA->PushString("missing params: use git.Open(path)");
	return 1;
}

int Git_CloneRepo( lua_State* state )
{
	if ( LUA->IsType( 1, Type::STRING ) && LUA->IsType( 2, Type::STRING ) )
	{
		const char* url = LUA->GetString( 1 );
		const char* path = LUA->GetString( 2 );
		git_repository *repo;
		//git_clone_options *clone_opts;

		int error = git_clone(&repo, url, path, NULL);
		CHECK_GIT_ERR(error);

		GarrysMod::Lua::UserData* ud = ( GarrysMod::Lua::UserData* )LUA->NewUserdata( sizeof( GarrysMod::Lua::UserData ) );
		ud->data = repo;
		ud->type = GarrysMod::Lua::Type::USERDATA; // ???
		LUA->ReferencePush( metatable_gitrepo );
		LUA->SetMetaTable( -2 );

		return 1;
	}

	LUA->PushString("missing params: use git.Clone(url, path)");
	return 1;
}

int Git_IsRepository( lua_State* state )
{
	if ( LUA->IsType( 1, Type::STRING ))
	{
		const char* path = LUA->GetString( 1 );
		
		LUA->PushBool(git_repository_open_ext(NULL, path, GIT_REPOSITORY_OPEN_NO_SEARCH, NULL) == 0);

		return 1;
	}

	LUA->PushString("missing params: use git.IsRepository(path)");
	return 1;
}

int Git_Fetch( lua_State* state )
{
	if (LUA->IsType( 1, Type::USERDATA ) )
	{
		GarrysMod::Lua::UserData* obj = ( GarrysMod::Lua::UserData* )LUA->GetUserdata( 1 );
		git_repository* repo = ( git_repository* )( obj->data );
		
		const char* remotename = LUA->IsType(2, Type::STRING) ? LUA->GetString( 2 ) : "origin";
		
		git_remote *remote = NULL;
		int error = git_remote_load(&remote, repo, remotename);

		CHECK_GIT_ERR(error);

		error = git_remote_fetch(remote, NULL, NULL, NULL);

		git_remote_free(remote);
		
		CHECK_GIT_ERR(error);

		LUA->PushBool(true);
		return 1;
	}
	LUA->PushString("missing params: use repo:Fetch([remotename])");
	return 1;
}

int git_push_status_foreach_cb(const char* ref, const char* msg, void* data) {
	//GMod_DebugMsg(ref, (lua_State*) data);
	//GMod_DebugMsg(msg, (lua_State*) data);
	return 0;
}
int git_push_transfer_progress_cb(
	unsigned int current,
	unsigned int total,
	size_t bytes,
	void* payload) {

	/*char str[100];
	sprintf(str,"%d/%d",current, total);
	GMod_DebugMsg(str, (lua_State*) payload);*/
	return 0;
}

int Git_Push( lua_State* state )
{
	if (LUA->IsType( 1, Type::USERDATA ) )
	{
		GarrysMod::Lua::UserData* obj = ( GarrysMod::Lua::UserData* )LUA->GetUserdata( 1 );
		git_repository* repo = ( git_repository* )( obj->data );
		
		const char* remotename = LUA->IsType(2, Type::STRING) ? LUA->GetString( 2 ) : "origin";
		
		git_remote *remote = NULL;
		int error = git_remote_load(&remote, repo, remotename);
		CHECK_GIT_ERR(error);

		// Connect to remote
		error = git_remote_connect(remote, GIT_DIRECTION_PUSH);
		CHECK_GIT_ERR(error);

		// Create push object
		git_push *push = NULL;
		error = git_push_new(&push, remote);
		CHECK_GIT_ERR(error);

		error = git_push_set_callbacks(push, NULL, NULL, &git_push_transfer_progress_cb, (void*) state);
		CHECK_GIT_ERR(error);
		
		// Add remote's refspecs to the push object
		/*
		git_strarray fetch_refspecs = {0};
		error = git_remote_get_fetch_refspecs(&fetch_refspecs, remote);
		CHECK_GIT_ERR(error);

		git_strarray push_refspecs = {0};
		error = git_remote_get_push_refspecs(&push_refspecs, remote);
		CHECK_GIT_ERR(error);

		// If there are no push refspecs, lets use the fetch refspec
		if (push_refspecs.count == 0) {
			if (fetch_refspecs.count == 0) {
				LUA->PushBool(false);
				LUA->PushString("No fetch nor push refspecs. Cant push.");
				return 2;
			}
			// Add push refspec from the first fetch refspec
			error = git_remote_add_push(remote, fetch_refspecs.strings[0]);
			CHECK_GIT_ERR(error);

			// Re-get push refspecs that should have one entry now
			error = git_remote_get_push_refspecs(&push_refspecs, remote);
			CHECK_GIT_ERR(error);
		}

		if (push_refspecs.count == 0) {
			LUA->PushBool(false);
			LUA->PushString("Zero refspecs to push (even after trying to get one from fetch refspecs)");
			return 2;
		}
		for (size_t i = 0; i < push_refspecs.count; ++i) {
			const char* rs = push_refspecs.strings[i];
			GMod_DebugMsg(rs, state);
			git_push_add_refspec(push, rs);
		}*/

		git_push_add_refspec(push, "refs/heads/master:refs/heads/master");

		error = git_push_finish(push);
		CHECK_GIT_ERR(error);

		if (!git_push_unpack_ok(push)) {
			LUA->PushBool(false);
			LUA->PushString("Unpacking failed");
			return 2;
		}
		
		error = git_push_status_foreach(push, &git_push_status_foreach_cb,(void*) state);
		CHECK_GIT_ERR(error);

		git_signature *me = NULL;
		error = git_signature_now(&me, "Me", "me@example.com");
		CHECK_GIT_ERR(error);

		error = git_push_update_tips(push, me, NULL);
		CHECK_GIT_ERR(error);

		git_push_free(push);
		git_remote_free(remote);

		LUA->PushBool(true);
		return 1;
	}
	LUA->PushString("missing params: use repo:Push([remotename])");
	return 1;
}

int Util_Commit(lua_State* state, git_repository* repo, const char* commitmsg) {
	git_signature *me = NULL;
	int error = git_signature_now(&me, "Me", "me@example.com");
	CHECK_GIT_ERR(error);
	assert(me);

	/*
	git_oid *parent_oid = NULL;
	error = git_reference_name_to_id(parent_oid, repo, "refs/heads/master");
	CHECK_GIT_ERR(error);
	assert(parent_oid);

	git_commit *commit;
	error = git_commit_lookup(&commit, repo, parent_oid);
	CHECK_GIT_ERR(error);
	assert(commit);
	*/
		
	//get last commit => parent

	git_object *git_obj = NULL;

	error = git_revparse_single(&git_obj, repo, "HEAD");
	CHECK_GIT_ERR(error);

	git_oid *parent_oid = (git_oid *)git_obj; 

	git_commit *parent;

	error = git_commit_lookup(&parent, repo, parent_oid);
	CHECK_GIT_ERR(error);

	// Get tree

	git_index *index = NULL;
	error = git_repository_index(&index, repo);
	CHECK_GIT_ERR(error);
	assert(index);

	git_oid tree_oid;
	error = git_index_write_tree(&tree_oid, index);
	CHECK_GIT_ERR(error);
	assert(tree_oid);
		
	git_tree *tree = NULL;
	error = git_tree_lookup(&tree, repo, &tree_oid);
	CHECK_GIT_ERR(error);
	assert(tree);

	const git_commit *parents[] = {parent};

	git_oid new_commit_id;
	error = git_commit_create(
		&new_commit_id,
		repo,
		"HEAD",                      /* name of ref to update */
		me,                          /* author */
		me,                          /* committer */
		"UTF-8",                     /* message encoding */
		commitmsg,  /* message */
		tree,                        /* root tree */
		1,                           /* parent count */
		parents);                    /* parents */
	CHECK_GIT_ERR(error);

	LUA->PushBool(true);
	return 1;
}

struct MHCBData {
	git_repository* repo;
	std::vector<git_annotated_commit*> *merge_heads;

	lua_State* state;

	const char* branch;
	const char* target_ref;
};

int Git_MergeHeadCallback(
	const char* ref_name,
	const char* remote_url,
	const git_oid* oid,
	unsigned int is_merge,
	void *payload)
{
	MHCBData* data = static_cast<MHCBData*>(payload);

	//GMod_DebugMsg(ref_name, data->state);
	
	// We only care about fetchheads that are for merge or are wanted branch name
	if (is_merge || strcmp(ref_name, data->target_ref) == 0) {

		git_annotated_commit *their_heads = NULL;
		int error = git_annotated_commit_from_fetchhead(&their_heads, data->repo, data->branch, remote_url, oid);

		(*data->merge_heads).push_back(their_heads);
	}

	return 0;
}

int Git_Merge( lua_State* state )
{
	if (LUA->IsType( 1, Type::USERDATA ) )
	{
		GarrysMod::Lua::UserData* obj = ( GarrysMod::Lua::UserData* )LUA->GetUserdata( 1 );
		git_repository* repo = ( git_repository* )( obj->data );

		const char* target_ref = "refs/heads/master";
		const char* branch = "master"; // what branch we want to merge to
		
		git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
		git_checkout_options co_opts = GIT_CHECKOUT_OPTIONS_INIT;
		
		co_opts.checkout_strategy = GIT_CHECKOUT_SAFE;

		std::vector<git_annotated_commit*> merge_heads;

		MHCBData data;
		data.merge_heads = &merge_heads;
		data.repo = repo;
		data.target_ref = target_ref;
		data.branch = branch;
		data.state = state;

		int error = git_repository_fetchhead_foreach(repo, &Git_MergeHeadCallback, static_cast<void*>(&data));
		CHECK_GIT_ERR(error);

		if (merge_heads.size() == 0) {
			LUA->PushBool(false);
			LUA->PushString("Zero mergeheads. No merge will happen");
			return 2;
		}

		// If our local repo is empty (aka has no branch, not even master) we need to create it for the merge to work
		git_reference *ref = NULL;
		int branchstatus = git_branch_lookup(&ref, repo, branch, GIT_BRANCH_LOCAL);
		if (branchstatus == GIT_ENOTFOUND) {
			//git_branch_create(&ref, repo, branch, NULL, 0, _, NULL);
		}

		git_reference_free(ref);

		git_annotated_commit** pointer = merge_heads.data();
		const git_annotated_commit** p = const_cast<const git_annotated_commit**>(pointer);

		error = git_merge(repo, p, merge_heads.size(), &merge_opts, &co_opts);
		CHECK_GIT_ERR(error);

		// Done with merge. Changes are staged, now we just need to make an actual commit
		
		return Util_Commit(state, repo, "Merging from upstream");
	}
	LUA->PushString("missing params: use repo:Merge([remote_branch])");
	return 1;
}

int Git_Add( lua_State* state )
{
	if (LUA->IsType( 1, Type::USERDATA ) && LUA->IsType( 2, Type::STRING )  )
	{
		GarrysMod::Lua::UserData* obj = ( GarrysMod::Lua::UserData* )LUA->GetUserdata( 1 );
		git_repository* repo = ( git_repository* )( obj->data );
		
		const char* filename = LUA->GetString( 2 );

		git_index *idx = NULL;
		int error = git_repository_index(&idx, repo);
		CHECK_GIT_ERR(error);

		error = git_index_add_bypath(idx, filename);
		CHECK_GIT_ERR(error);

		// Update index on disk
		error = git_index_write(idx);
		CHECK_GIT_ERR(error);

		git_index_free(idx);

		return 0;
	}
	LUA->PushString("missing params: use repo:Add(filename)");
	return 1;
}

int Git_Remove( lua_State* state )
{
	if (LUA->IsType( 1, Type::USERDATA ) && LUA->IsType( 2, Type::STRING )  )
	{
		GarrysMod::Lua::UserData* obj = ( GarrysMod::Lua::UserData* )LUA->GetUserdata( 1 );
		git_repository* repo = ( git_repository* )( obj->data );
		
		const char* filename = LUA->GetString( 2 );

		git_index *idx = NULL;
		int error = git_repository_index(&idx, repo);
		CHECK_GIT_ERR(error);

		error = git_index_remove_bypath(idx, filename);
		CHECK_GIT_ERR(error);
		
		// Update index on disk
		error = git_index_write(idx);
		CHECK_GIT_ERR(error);

		git_index_free(idx);

		return 0;
	}
	LUA->PushString("missing params: use repo:Remove(filename)");
	return 1;
}

int Git_Commit( lua_State* state )
{
	if (LUA->IsType( 1, Type::USERDATA ) && LUA->IsType( 2, Type::STRING )  )
	{
		GarrysMod::Lua::UserData* obj = ( GarrysMod::Lua::UserData* )LUA->GetUserdata( 1 );
		git_repository* repo = ( git_repository* )( obj->data );
		
		const char* commitmsg = LUA->GetString( 2 );

		return Util_Commit(state, repo, commitmsg);
	}
	LUA->PushString("missing params: use repo:Commit(message)");
	return 1;
}

#define SET_BOOL_IN_TABLE(k, v) LUA->PushString(k);\
								LUA->PushBool(v);\
								LUA->SetTable(-3);


// lua_run require("luagit") repo = git.Open("urascrub")
int Git_Status( lua_State* state )
{
	if (LUA->IsType( 1, Type::USERDATA ) )
	{
		GarrysMod::Lua::UserData* obj = ( GarrysMod::Lua::UserData* )LUA->GetUserdata( 1 );
		git_repository* repo = ( git_repository* )( obj->data );

		int error;
		
		LUA->CreateTable();

			// Show current branch
			git_reference *head = NULL;
			const char* branch = NULL;
			error = git_repository_head(&head, repo);

			if (error == GIT_EUNBORNBRANCH || error == GIT_ENOTFOUND) {
				branch = NULL;
			}
			else if (!error) {
				branch = git_reference_shorthand(head);
			}
			else {
				CHECK_GIT_ERR(error);
			}

			LUA->PushString( "Branch" );
			if (branch)
				LUA->PushString( branch );
			else
				LUA->PushNil();
			LUA->SetTable( -3 );
			
			// Find changes

			git_status_options opts = GIT_STATUS_OPTIONS_INIT;
			opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
			opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
				GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX |
				GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;

			git_status_list *statuses = NULL;
			error = git_status_list_new(&statuses, repo, &opts);
			CHECK_GIT_ERR(error);

			size_t count = git_status_list_entrycount(statuses);

			const char *old_path, *new_path;

			// Create a table to show filechange status

			LUA->PushString("IndexChanges");
			LUA->CreateTable();
			for (size_t i=0; i<count; ++i) {
				char *istatus = NULL;

				const git_status_entry *s = git_status_byindex(statuses, i);

				if (s->status == GIT_STATUS_CURRENT)
					continue;

				if (s->status & GIT_STATUS_INDEX_NEW)
					istatus = "new file";
				if (s->status & GIT_STATUS_INDEX_MODIFIED)
					istatus = "modified";
				if (s->status & GIT_STATUS_INDEX_DELETED)
					istatus = "deleted";
				if (s->status & GIT_STATUS_INDEX_RENAMED)
					istatus = "renamed";
				if (s->status & GIT_STATUS_INDEX_TYPECHANGE)
					istatus = "typechange";

				if (istatus == NULL)
					continue;

				old_path = s->head_to_index->old_file.path;
				new_path = s->head_to_index->new_file.path;
					
				LUA->PushNumber(i+1);
				LUA->CreateTable();
					
				LUA->PushString("Status");
				LUA->PushString(istatus);
				LUA->SetTable( -3 );

				if (old_path && new_path && strcmp(old_path, new_path)) {
					LUA->PushString("OldPath");
					LUA->PushString(old_path);
					LUA->SetTable( -3 );

					LUA->PushString("NewPath");
					LUA->PushString(new_path);
					LUA->SetTable( -3 );
				}
					
				LUA->PushString("Path");
				LUA->PushString(new_path ? new_path : old_path);
				LUA->SetTable( -3 );
					
				LUA->SetTable(-3);
			}
			LUA->SetTable( -3 );

			LUA->PushString("WorkDirChanges");
			LUA->CreateTable();
			for (size_t i=0; i<count; ++i) {
				char *wstatus = NULL;

				const git_status_entry *s = git_status_byindex(statuses, i);

				if (s->status == GIT_STATUS_CURRENT || s->index_to_workdir == NULL)
					continue;

				if (s->status & GIT_STATUS_WT_MODIFIED)
					wstatus = "modified";
				if (s->status & GIT_STATUS_WT_DELETED)
					wstatus = "deleted";
				if (s->status & GIT_STATUS_WT_RENAMED)
					wstatus = "renamed";
				if (s->status & GIT_STATUS_WT_TYPECHANGE)
					wstatus = "typechange";

				if (wstatus == NULL)
					continue;
				
				old_path = s->index_to_workdir->old_file.path;
				new_path = s->index_to_workdir->new_file.path;

				LUA->PushNumber(i+1);
				LUA->CreateTable();
					
				LUA->PushString("Status");
				LUA->PushString(wstatus);
				LUA->SetTable( -3 );

				if (old_path && new_path && strcmp(old_path, new_path)) {
					LUA->PushString("OldPath");
					LUA->PushString(old_path);
					LUA->SetTable( -3 );

					LUA->PushString("NewPath");
					LUA->PushString(new_path);
					LUA->SetTable( -3 );
				}
					
				LUA->PushString("Path");
				LUA->PushString(new_path ? new_path : old_path);
				LUA->SetTable( -3 );
					
				LUA->SetTable(-3);
			}
			LUA->SetTable(-3);
			
			LUA->PushString("UntrackedFiles");
			LUA->CreateTable();
			for (size_t i=0; i<count; ++i) {
				const git_status_entry *s = git_status_byindex(statuses, i);
				
				if (s->status == GIT_STATUS_WT_NEW) {
					LUA->PushNumber(i+1);
					LUA->PushString(s->index_to_workdir->old_file.path);
					LUA->SetTable(-3);
				}
			}

			LUA->SetTable( -3 );
		return 1;
	}
	LUA->PushString("missing params: use repo:Status()");
	return 1;
}

int Git_FreeRepo( lua_State* state )
{
	if (LUA->IsType( 1, Type::USERDATA ) )
	{
		GarrysMod::Lua::UserData* obj = ( GarrysMod::Lua::UserData* )LUA->GetUserdata( 1 );
		git_repository* repo = ( git_repository* )( obj->data );
		git_repository_free(repo);
		return 0;
	}
	LUA->PushString("missing params: use repo:Free(), not repo.Free()");
	return 1;
}

int indexWrapper( lua_State* state )
{
	if ( !LUA->IsType(1, GarrysMod::Lua::Type::USERDATA ) || !LUA->IsType( 2, Type::STRING ) ) {
		return 0;
	}
	GarrysMod::Lua::UserData* obj = ( GarrysMod::Lua::UserData* )LUA->GetUserdata( 1 );
    git_repository* repo = ( git_repository* )( obj->data );
	const char* keyName = LUA->GetString(2);

	if (strcmp(keyName, "Free") == 0) {
		LUA->PushCFunction( Git_FreeRepo );
		return 1;
	}
	else if (strcmp(keyName, "Add") == 0) {
		LUA->PushCFunction( Git_Add );
		return 1;
	}
	else if (strcmp(keyName, "Remove") == 0) {
		LUA->PushCFunction( Git_Remove );
		return 1;
	}
	else if (strcmp(keyName, "Fetch") == 0) {
		LUA->PushCFunction( Git_Fetch );
		return 1;
	}
	else if (strcmp(keyName, "Merge") == 0) {
		LUA->PushCFunction( Git_Merge );
		return 1;
	}
	else if (strcmp(keyName, "Push") == 0) {
		LUA->PushCFunction( Git_Push );
		return 1;
	}
	else if (strcmp(keyName, "Commit") == 0) {
		LUA->PushCFunction( Git_Commit );
		return 1;
	}
	else if (strcmp(keyName, "Status") == 0) {
		LUA->PushCFunction( Git_Status );
		return 1;
	}

	return 0;
}
