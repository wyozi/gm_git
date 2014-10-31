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

RepositoryStatus* Repository::GetStatus() {
	RepositoryStatus* status;

	git_reference* head;

	int error = git_repository_head(&head, repo);
	if (error < 0) throw GitError(error);

	status->branch = std::string(git_reference_shorthand(head));

	return status;
}

void Repository::Free() {
	git_repository_free(repo);
}
