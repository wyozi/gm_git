#pragma once

#include <stdlib.h>
#include <functional>

#include "Util.h"

namespace ChangeType {
	enum {
		NEW,
		MODIFIED,
		DELETED,
		RENAMED,
		TYPECHANGE
	};
}

struct RepositoryStatusEntry {
	std::string path;
	std::string old_path;
	int type;
};
struct RepositoryStatus {
	std::string branch;
	std::vector<RepositoryStatusEntry*> index_changes;
	std::vector<RepositoryStatusEntry*> work_dir_changes;
};

class Repository {
public:
	Repository(std::string repo_path);
	~Repository();

	RepositoryStatus* GetStatus();
	void Free();
	
private:
	std::string path;
	git_repository *repo;

};

struct GitError : std::exception {
    public:
		int error;

        GitError(int errid) : error(errid){}
};