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
		switch(s->status) {
		case GIT_STATUS_INDEX_NEW:
			type = ChangeType::enum_strings[ChangeType::NEW]; break;
		case GIT_STATUS_INDEX_MODIFIED:
			type = ChangeType::enum_strings[ChangeType::MODIFIED]; break;
		case GIT_STATUS_INDEX_DELETED:
			type = ChangeType::enum_strings[ChangeType::DELETED]; break;
		case GIT_STATUS_INDEX_RENAMED:
			type = ChangeType::enum_strings[ChangeType::RENAMED]; break;
		case GIT_STATUS_INDEX_TYPECHANGE:
			type = ChangeType::enum_strings[ChangeType::TYPECHANGE]; break;
		}

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
		switch(s->status) {
		case GIT_STATUS_WT_NEW:
			type = ChangeType::enum_strings[ChangeType::NEW]; break;
		case GIT_STATUS_WT_MODIFIED:
			type = ChangeType::enum_strings[ChangeType::MODIFIED]; break;
		case GIT_STATUS_WT_DELETED:
			type = ChangeType::enum_strings[ChangeType::DELETED]; break;
		case GIT_STATUS_WT_RENAMED:
			type = ChangeType::enum_strings[ChangeType::RENAMED]; break;
		case GIT_STATUS_WT_TYPECHANGE:
			type = ChangeType::enum_strings[ChangeType::TYPECHANGE]; break;
		}

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

void Repository::Free() {
	git_repository_free(repo);
}
