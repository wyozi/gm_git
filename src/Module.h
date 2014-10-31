#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdio>
#include <vector>
#include <sstream>
#include <iostream>
#include <cassert>

#ifdef _WIN32
    #include <direct.h>
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
#endif

#include "GarrysMod/Lua/Interface.h"

#include "git2.h"

#include "Util.h"
#include "Repository.h"
#include "LuaBridge.h"

namespace Wyozi {
	enum {
		TYPE_REPOSITORY = GarrysMod::Lua::Type::COUNT + 1,

		COUNT,
	};
};