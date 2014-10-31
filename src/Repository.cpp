#include "Repository.h"

Repository::Repository(std::string repo_path) : path(repo_path) {
	int error = git_repository_open(&repo, path.c_str());
	if (error < 0) {
		throw GitError(error);
	}
}

Repository::~Repository() {
	this->Free();
}

void Repository::Fetch(std::string remotename) {
	git_remote* remote = NULL;
	int error = git_remote_load(&remote, repo, remotename.c_str());
	if (error < 0) throw GitError(error);

	error = git_remote_fetch(remote, NULL, NULL, NULL);
	if (error < 0) throw GitError(error);

	git_remote_free(remote);
}

void Repository::Push(std::string remotename) {
	git_remote* remote = NULL;
	int error = git_remote_load(&remote, repo, remotename.c_str());
	if (error < 0) throw GitError(error);
	
	// Connect to remote
	error = git_remote_connect(remote, GIT_DIRECTION_PUSH);
	if (error < 0) throw GitError(error);

	// Create push object
	git_push *push = NULL;
	error = git_push_new(&push, remote);
	if (error < 0) throw GitError(error);
	
	// TODO this needs to be settable by the user somehow
	git_push_add_refspec(push, "refs/heads/master:refs/heads/master");

	error = git_push_finish(push);
	if (error < 0) throw GitError(error);

	error = git_push_unpack_ok(push);
	if (error < 0) throw GitError(error);

	// TODO this needs to be settable by the user somehow. Probably SetSignature/GetSignature
	git_signature *me = NULL;
	error = git_signature_now(&me, "Me", "me@example.com");
	if (error < 0) throw GitError(error);

	error = git_push_update_tips(push, me, NULL);
	if (error < 0) throw GitError(error);

	git_push_free(push);
	git_remote_free(remote);
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

void Repository::AddToIndex(std::string path) {
	git_index *idx = NULL;
	int error = git_repository_index(&idx, repo);
	if (error < 0) throw GitError(error);

	error = git_index_add_bypath(idx, path.c_str());
	if (error < 0) throw GitError(error);

	error = git_index_write(idx);
	if (error < 0) throw GitError(error);
}

void Repository::RemoveFromIndex(std::string path) {
	git_index *idx = NULL;
	int error = git_repository_index(&idx, repo);
	if (error < 0) throw GitError(error);

	error = git_index_remove_bypath(idx, path.c_str());
	if (error < 0) throw GitError(error);

	error = git_index_write(idx);
	if (error < 0) throw GitError(error);
}


void Repository::Free() {
	git_repository_free(repo);
}
