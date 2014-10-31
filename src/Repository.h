#pragma once

#include <stdlib.h>
#include "Util.h"

struct RepositoryStatus {
	std::string branch;
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