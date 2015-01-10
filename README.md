gm_git
======

## What?

gm_git is a binary module for Garry's Mod, that makes it possible to fetch, merge, push, commit etc git repositories. See https://github.com/wyozi/g-ace for an example usecase.

## Usage

API is not stable. See [LuaBridge.cpp](../master/src/LuaBridge.cpp).

## Building

### Requirements
You need to build libgit2 before building the Garry's Mod module. After cloning gm_git, run ```git submodule update```, and then follow the instructions at https://libgit2.github.com/docs/guides/build-and-link/

You also need premake4. There is a premake4.exe included for windows in the repository.

### Windows  

- run ```BuildProjects.bat```  
- open ```windows-vs2010``` folder and build the project in Visual Studio
- there should be a ```gmsv_luagit_win32.dll``` in ```builds/windows```

__Note:__ When compiling libgit2 statically (using BUILD_SHARED_LIBS=OFF) on Windows, remember to change Runtime Library to Multi Threaded Debug or you can just build a non-debug version of libgit2.

### Linux
- run ```sh BuildProjects.bat``` (don't worry it will work)
- open ```linux-gmake``` and run ```make```
- there should be a ```libgmsv_luagit_linux.so``` in ```builds/linux```, rename that to ```gmsv_luagit_linux.dll``` and it is usable as a Garry's Mod module


### git2.dll or libgit2.so
If you did not set libgit2 to produce static libraries, you're gonna need the libgit2 dynamic library in steamapps/common/GarrysMod/ on clients and similar place on servers.
