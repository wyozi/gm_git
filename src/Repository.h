#pragma once

#include <stdlib.h>
#include <functional>
#include <map>

#include "Util.h"

namespace ChangeType {
	enum {
		NEW = 1,
		MODIFIED,
		DELETED,
		RENAMED,
		TYPECHANGE
	};

	const std::string enum_strings[] = {"", "new", "modified", "deleted", "renamed", "typechange"};
}

enum StatusCode {
	GMGIT_OK = 0,

	// No changes in index
	GMGIT_COMMIT_NOCHANGES = 1
};

struct CommitOptions {
	std::string commitmsg;
	git_commit* merge_head;

	std::string committer_name;
	std::string committer_email;
};
struct MergeOptions {
	std::string branch;
	std::string commitmsg;
	std::vector<git_merge_head*> annotated_commits;
};

struct RepositoryStatusEntry {
	std::string path;
	std::string old_path;
	std::string type;
};
struct RepositoryStatus {
	std::string branch;
	std::vector<RepositoryStatusEntry*> index_changes;
	std::vector<RepositoryStatusEntry*> work_dir_changes;
};

struct RepositoryLogEntry {
	std::string ref;

	std::string commitmsg;

	std::string committer;
	std::string author;

};
struct RepositoryLog {
	std::vector<RepositoryLogEntry*> log_entries;
};

class Repository {
public:
	Repository(std::string repo_path);
	~Repository();

	void SetCredentials(std::string username, std::string password);
	std::string GetUsername();
	std::string GetPassword();

	void Fetch(std::string remotename = "origin");
	void Push(std::string remotename = "origin");

	int Commit(CommitOptions* commit_options);
	int Commit(std::string commitmsg = "");
	void Merge(MergeOptions* merge_options);

	void Pull(std::string remotename = "origin");

	unsigned int GetFileStatus(std::string path);
	RepositoryStatus* GetStatus();

	RepositoryLog* GetLog();

	std::vector<std::string> GetIndexEntries();
	void AddPathSpecToIndex(std::string pathspec);
	void AddIndexEntry(std::string path);
	void RemoveIndexEntry(std::string path);

	std::string DiffIndexToWorkdir();
	std::string DiffHEADToIndex();
	std::string DiffHEADToWorkdir();

	void Free();
	
private:
	git_repository *repo;

	std::string path;

	std::string username;
	std::string password;
};

struct GitError : std::exception {
    public:
		int error;

        GitError(int errid) : error(errid){}
};