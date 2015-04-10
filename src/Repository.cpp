#include "Repository.h"

Repository::Repository(std::string repo_path) : path(repo_path) {
	int error = git_repository_open(&repo, path.c_str());
	if (error < 0) throw GitError(error);
}

Repository::~Repository() {
	this->Free();
}

void Repository::SetCredentials(std::string username, std::string password) {
	this->username = username;
	this->password = password;
}
std::string Repository::GetUsername() {
	return this->username;
}
std::string Repository::GetPassword() {
	return this->password;
}

void Repository::Fetch(std::string remotename) {
	git_remote* remote = NULL;
	int error = git_remote_lookup(&remote, repo, remotename.c_str());
	if (error < 0) throw GitError(error);

	error = git_remote_fetch(remote, NULL, NULL, NULL);
	if (error < 0) throw GitError(error);

	git_remote_free(remote);
}

// Credential callback for remotes. Tries to get credentials from repo object
int credential_cb(git_cred **out,
                  const char *url,
                  const char *username_from_url,
                  unsigned int allowed_types,
                  void *payload)
{
	Repository *repo = (Repository*)payload;
	
	std::string username = repo->GetUsername();
	std::string password = repo->GetPassword();

	if (!username.empty()) {
		return git_cred_userpass_plaintext_new(out, username.c_str(), password.c_str());
	}

	return 1; // no cred was acquired
}
void Repository::Push(std::string remotename) {
	git_remote* remote = NULL;
	int error = git_remote_lookup(&remote, repo, remotename.c_str());
	if (error < 0) throw GitError(error);

	git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
	
	callbacks.credentials = credential_cb;
	callbacks.payload = this;
	error = git_remote_set_callbacks(remote, &callbacks);
	if (error < 0) throw GitError(error);

	// Connect to remote
	error = git_remote_connect(remote, GIT_DIRECTION_PUSH);
	if (error < 0) throw GitError(error);

	// Create refspec strarray
	char *refspecs_arr = "refs/heads/master:refs/heads/master";

	git_strarray* refspecs = new git_strarray;
	refspecs->count = 1;
	refspecs->strings = &refspecs_arr;

	// Create push options
	git_push_options* push_opts = NULL;
	error = git_push_init_options(push_opts, GIT_PUSH_OPTIONS_VERSION);
	if (error < 0) throw GitError(error);

	// Create push object
	error = git_remote_upload(remote, refspecs, push_opts);
	if (error < 0) throw GitError(error);

	git_signature *signature = NULL;

	error = git_signature_default(&signature, repo);
	if (error < 0) throw GitError(error);

	error = git_remote_update_tips(remote, signature, NULL);
	if (error < 0) throw GitError(error);

	git_remote_free(remote);
	git_signature_free(signature);
}

int Repository::Commit(CommitOptions* commit_options) {
	git_object *obj = NULL;

	// Compute diff between HEAD and index to see if there actually were any changes
	int error = git_revparse_single(&obj, repo, "HEAD^{tree}");
	if (error < 0) throw GitError(error);

	git_tree *head_tree = NULL;
	error = git_tree_lookup(&head_tree, repo, git_object_id(obj));
	if (error < 0) throw GitError(error);

	git_diff *diff = NULL;
	error = git_diff_tree_to_index(&diff, repo, head_tree, NULL, NULL);
	if (error < 0) throw GitError(error);

	// If zero deltas, then there we zero changes between index and HEAD?
	size_t deltas = git_diff_num_deltas(diff);
	
	// Cleanup
	git_tree_free(head_tree);
	git_diff_free(diff);

	if (deltas == 0) {
		return GMGIT_COMMIT_NOCHANGES;
	}

	// Get git_commit of HEAD, which is what we'll base the new commit on. TODO: what if merged?
	error = git_revparse_single(&obj, repo, "HEAD");
	if (error < 0) throw GitError(error);

	git_oid *head_oid = (git_oid *)obj; 

	git_commit *head;
	error = git_commit_lookup(&head, repo, head_oid);
	if (error < 0) throw GitError(error);
	
	// Get tree of index
	git_index *index = NULL;
	error = git_repository_index(&index, repo);
	if (error < 0) throw GitError(error);

	git_oid tree_oid;
	error = git_index_write_tree(&tree_oid, index);
	if (error < 0) throw GitError(error);
		
	git_tree *index_tree = NULL;
	error = git_tree_lookup(&index_tree, repo, &tree_oid);
	if (error < 0) throw GitError(error);

	// Get parents
	std::vector<git_commit*> parents;
	parents.push_back(head);
	if (commit_options && commit_options->merge_head) {
		parents.push_back(commit_options->merge_head);
	}

	const git_commit **parent_pointer = const_cast<const git_commit**>(parents.data());

	// Get signature
	git_signature* signature;
	if (!commit_options->committer_name.empty()) {
		error = git_signature_now(&signature, commit_options->committer_name.c_str(), commit_options->committer_email.c_str());
	}
	else {
		error = git_signature_default(&signature, repo);
	}
	if (error < 0) throw GitError(error);

	git_oid new_commit_id;
	error = git_commit_create(
		&new_commit_id,
		repo,
		"HEAD",                      /* name of ref to update */
		signature,                          /* author */
		signature,                          /* committer */
		"UTF-8",                     /* message encoding */
		commit_options->commitmsg.c_str(),  /* message */
		index_tree,                        /* root tree */
		parents.size(),                           /* parent count */
		parent_pointer);                    /* parents */
	if (error < 0) throw GitError(error);

	git_index_free(index);
	git_commit_free(head);
	git_tree_free(index_tree);
	git_signature_free(signature);

	return GMGIT_OK;
}

void Repository::Merge(MergeOptions* merge_options) {
	git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
	git_checkout_options co_opts = GIT_CHECKOUT_OPTIONS_INIT;
	
	co_opts.checkout_strategy = GIT_CHECKOUT_SAFE_CREATE;
	
	git_annotated_commit** pointer = merge_options->annotated_commits.data();
	const git_annotated_commit** const_pointer = const_cast<const git_annotated_commit**>(pointer);

	std::cout << &pointer << std::endl;

	int error = git_merge(repo, const_pointer, merge_options->annotated_commits.size(), &merge_opts, &co_opts);
	if (error < 0) throw GitError(error);

	std::string commitmsg = merge_options->commitmsg.empty() ? "Merged" : merge_options->commitmsg;
	
	CommitOptions opt = {commitmsg};

	// TODO Not sure how git works, but we get the first viable git_merge_head from the vector and use the ref from that
	if (merge_options->annotated_commits.size() > 0) {
		const git_oid* commit_oid = git_annotated_commit_id(pointer[0]);

		git_commit *merge_head;
		error = git_commit_lookup(&merge_head, repo, commit_oid);
		if (error < 0) throw GitError(error);

		opt.merge_head = merge_head;
	}

	error = git_repository_state_cleanup(repo);
	if (error < 0) throw GitError(error);

	this->Commit(&opt);
}

struct fetchhead_cb_data {
	git_repository* repo;
	const char* branch;
	std::vector<git_annotated_commit*> annotated_commits;
};

int fetchhead_callback(const char* ref_name, const char* remote_url, const git_oid* oid, unsigned int is_merge, void *payload) {
	fetchhead_cb_data* cbdata = static_cast<fetchhead_cb_data*>(payload);

	if (is_merge) {
		git_annotated_commit* commit;

		int error = git_annotated_commit_from_fetchhead(&commit, cbdata->repo, cbdata->branch, remote_url, oid);
		if (error < 0) throw GitError(error);

		cbdata->annotated_commits.push_back(commit);
	}

	return 0;
}

void Repository::Pull(std::string remotename) {
	this->Fetch(remotename);
	
	// Get current branch
	git_reference* head;
	int error = git_repository_head(&head, repo);
	if (error < 0) throw GitError(error);

	const char* branch = git_reference_shorthand(head);

	// Get fetchheads and turn them into annotated commits

	fetchhead_cb_data* cbdata = new fetchhead_cb_data;

	cbdata->repo = repo;
	cbdata->branch = branch;

	git_repository_fetchhead_foreach(repo, &fetchhead_callback, static_cast<void*>(cbdata));
	
	std::vector<git_annotated_commit*> annotated_commits = cbdata->annotated_commits;

	MergeOptions* merge_options = new MergeOptions;

	// insert all of our commits into the merge options
	merge_options->annotated_commits.insert(merge_options->annotated_commits.end(), annotated_commits.begin(), annotated_commits.end());

	this->Merge(merge_options);
}

std::vector<RepositoryStatusEntry*> GetFilteredStatuses(git_status_list* list, std::function<bool(const git_status_entry*, RepositoryStatusEntry*)> f) {
	std::vector<RepositoryStatusEntry*> vec;
	size_t count = git_status_list_entrycount(list);
	
	for (size_t i=0; i<count; ++i) {
		const git_status_entry *s = git_status_byindex(list, i);
		RepositoryStatusEntry* entry = new RepositoryStatusEntry;
		if (f(s, entry))
			vec.push_back(entry);
	}

	return vec;
}

unsigned int Repository::GetFileStatus(std::string path) {
	unsigned int f = 0;
	unsigned int* flags = &f;

	int error = git_status_file(flags, repo, path.c_str());
	if (error < 0) throw GitError(error);

	return f;
}

RepositoryStatus* Repository::GetStatus() {
	RepositoryStatus* status = new RepositoryStatus;
	int error;

	// Get current branch
	git_reference* head;
	error = git_repository_head(&head, repo);
	if (error < 0) throw GitError(error);

	status->branch = std::string(git_reference_shorthand(head));

	// Get all changes in index, workdir etc
	git_status_options opts = GIT_STATUS_OPTIONS_INIT;
	opts.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
	opts.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED |
				 GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX |
				 GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;

	git_status_list *statuses = NULL;
	error = git_status_list_new(&statuses, repo, &opts);
	if (error < 0) throw GitError(error);

	// Index changes (aka changes to be committed)
	status->index_changes = GetFilteredStatuses(statuses, [](const git_status_entry* s, RepositoryStatusEntry* rse) -> bool {
		if (s->status == GIT_STATUS_CURRENT)
			return false;

		std::string type;
		if (s->status & GIT_STATUS_INDEX_NEW)
			type = ChangeType::enum_strings[ChangeType::NEW];
		if (s->status & GIT_STATUS_INDEX_MODIFIED)
			type = ChangeType::enum_strings[ChangeType::MODIFIED];
		if (s->status & GIT_STATUS_INDEX_DELETED)
			type = ChangeType::enum_strings[ChangeType::DELETED];
		if (s->status & GIT_STATUS_INDEX_RENAMED)
			type = ChangeType::enum_strings[ChangeType::RENAMED];
		if (s->status & GIT_STATUS_INDEX_TYPECHANGE)
			type = ChangeType::enum_strings[ChangeType::TYPECHANGE];

		if (type.empty())
			return false;

		rse->type = type;

		rse->path = s->head_to_index->new_file.path;
		rse->old_path = s->head_to_index->old_file.path;

		return true;
	});
	
	// Workdirectory changes (aka changes that are _not_ staged for commit)
	// Note: workdir changes with status of GIT_STATUS_WT_NEW are untracked
	status->work_dir_changes = GetFilteredStatuses(statuses, [](const git_status_entry* s, RepositoryStatusEntry* rse) -> bool {
		if (s->status == GIT_STATUS_CURRENT || s->index_to_workdir == NULL)
			return false;
		
		std::string type;
		if (s->status & GIT_STATUS_WT_NEW)
			type = ChangeType::enum_strings[ChangeType::NEW];
		if (s->status & GIT_STATUS_WT_MODIFIED)
			type = ChangeType::enum_strings[ChangeType::MODIFIED];
		if (s->status & GIT_STATUS_WT_DELETED)
			type = ChangeType::enum_strings[ChangeType::DELETED];
		if (s->status & GIT_STATUS_WT_RENAMED)
			type = ChangeType::enum_strings[ChangeType::RENAMED];
		if (s->status & GIT_STATUS_WT_TYPECHANGE)
			type = ChangeType::enum_strings[ChangeType::TYPECHANGE];

		if (type.empty())
			return false;

		rse->type = type;

		rse->path = s->index_to_workdir->new_file.path;
		rse->old_path = s->index_to_workdir->old_file.path;

		return true;
	});

	git_status_list_free(statuses);

	return status;
}

RepositoryLog* Repository::GetLog() {

	std::vector<RepositoryLogEntry*> entries;

	git_revwalk *walker;
	int error = git_revwalk_new(&walker, repo);
	
	error = git_revwalk_push_head(walker);
	//error = git_revwalk_push_range(walker, "HEAD~20..HEAD");
	if (error < 0) throw GitError(error);

	git_oid oid;
	while (!git_revwalk_next(&oid, walker)) {
		git_commit *commit;
		error = git_commit_lookup(&commit, repo, &oid);
		if (error < 0) throw GitError(error);

		RepositoryLogEntry* entry = new RepositoryLogEntry;

		// Oid as a 40 char SHA-1 char pointer
		const git_oid *oid = git_commit_id(commit);

		char *hex_string = new char[41];
		hex_string[40] = '\0'; // Null terminate; not done by libgit2
		git_oid_fmt(hex_string, oid);

		entry->ref = std::string(hex_string);

		free(hex_string);

		// Commit message
		entry->commitmsg = git_commit_message(commit);

		// Committer
		const git_signature *committer = git_commit_committer(commit);
		std::ostringstream committerStream;
		committerStream << committer->name << " <" << committer->email << ">";

		entry->committer = committerStream.str();

		// Author
		const git_signature *author = git_commit_author(commit);
		std::ostringstream authorStream;
		authorStream << author->name << " <" << author->email << ">";

		entry->author = authorStream.str();

		entries.push_back(entry);

		if (entries.size() >= 20)
			break;
	}
	
	RepositoryLog* log = new RepositoryLog;
	log->log_entries = entries;

	git_revwalk_free(walker);

	return log;
}

std::vector<std::string> Repository::GetIndexEntries() {
	std::vector<std::string> entries;

	git_index *idx = NULL;
	int error = git_repository_index(&idx, repo);
	if (error < 0) throw GitError(error);

	size_t count = git_index_entrycount(idx);
	for (size_t i=0; i<count; i++) {
		const git_index_entry *entry = git_index_get_byindex(idx, i);
		entries.push_back(std::string(entry->path));
	}

	return entries;
}

void Repository::AddPathSpecToIndex(std::string pathspec) {
	char* pathspec_c = const_cast<char *>(pathspec.c_str());
	char *pathspecs[] = {pathspec_c};
	git_strarray arr = {pathspecs, 1};

	git_index *idx = NULL;
	int error = git_repository_index(&idx, repo);
	if (error < 0) throw GitError(error);

	error = git_index_add_all(idx, &arr, GIT_INDEX_ADD_DEFAULT, NULL, NULL);
	if (error < 0) throw GitError(error);
	
	error = git_index_write(idx);
	if (error < 0) throw GitError(error);
}

void Repository::AddIndexEntry(std::string path) {
	git_index *idx = NULL;
	int error = git_repository_index(&idx, repo);
	if (error < 0) throw GitError(error);

	error = git_index_add_bypath(idx, path.c_str());
	if (error < 0) throw GitError(error);

	error = git_index_write(idx);
	if (error < 0) throw GitError(error);
}

void Repository::RemoveIndexEntry(std::string path) {
	git_index *idx = NULL;
	int error = git_repository_index(&idx, repo);
	if (error < 0) throw GitError(error);

	error = git_index_remove_bypath(idx, path.c_str());
	if (error < 0) throw GitError(error);

	error = git_index_write(idx);
	if (error < 0) throw GitError(error);
}

struct diff_cb_data {
	std::ostringstream stream;
};

int diff_callback(const git_diff_delta *delta,
	const git_diff_hunk *hunk,
	const git_diff_line *line,
	void *payload)
{
	diff_cb_data* cbdata = static_cast<diff_cb_data*>(payload);

	std::string str = std::string(line->content, line->content_len);

	switch (line->origin) {
	case GIT_DIFF_LINE_ADDITION:
	case GIT_DIFF_LINE_ADD_EOFNL:
		cbdata->stream << "+";
		break;
	case GIT_DIFF_LINE_DELETION:
	case GIT_DIFF_LINE_DEL_EOFNL:
		cbdata->stream << "-";
		break;
	case GIT_DIFF_LINE_HUNK_HDR:
	case GIT_DIFF_LINE_FILE_HDR:
		// Add no space
		break;
	default:
		cbdata->stream << " ";
		break;
	}

	cbdata->stream << str;

	return 0;
}

std::string diffToString(git_diff* diff) {
	diff_cb_data* cbdata = new diff_cb_data;
	int error = git_diff_print(diff, GIT_DIFF_FORMAT_PATCH, &diff_callback, static_cast<void*>(cbdata));
	if (error < 0) throw GitError(error);

	std::string str = cbdata->stream.str();

	delete cbdata;

	return str;
}

std::string Repository::DiffIndexToWorkdir() {
	git_diff *diff = NULL;
	int error = git_diff_index_to_workdir(&diff, repo, NULL, NULL);
	if (error < 0) throw GitError(error);
	
	std::string str = diffToString(diff);

	git_diff_free(diff);
	return str;
}

std::string Repository::DiffHEADToIndex() {
	git_object *obj = NULL;
	int error = git_revparse_single(&obj, repo, "HEAD^{tree}");
		if (error < 0) throw GitError(error);

	git_tree *tree = NULL;
	error = git_tree_lookup(&tree, repo, git_object_id(obj));
		if (error < 0) throw GitError(error);

	git_diff *diff = NULL;
	error = git_diff_tree_to_index(&diff, repo, tree, NULL, NULL);
	if (error < 0) throw GitError(error);

	std::string str = diffToString(diff);

	git_diff_free(diff);
	return str;
}
std::string Repository::DiffHEADToWorkdir() {
	git_object *obj = NULL;
	int error = git_revparse_single(&obj, repo, "HEAD^{tree}");
	if (error < 0) throw GitError(error);

	git_tree *tree = NULL;
	error = git_tree_lookup(&tree, repo, git_object_id(obj));
	if (error < 0) throw GitError(error);

	git_diff *diff = NULL;
	error = git_diff_tree_to_workdir_with_index(&diff, repo, tree, NULL);
	if (error < 0) throw GitError(error);
	
	std::string str = diffToString(diff);

	git_diff_free(diff);
	return str;
}

void Repository::Free() {
	git_repository_free(repo);
}
