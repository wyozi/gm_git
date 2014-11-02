solution "gmsv_git"

	language "C++"
	location ( os.get() .."-".. _ACTION )
	flags { "Symbols", "NoEditAndContinue", "StaticRuntime", "NoPCH", "EnableSSE" } --
	targetdir ( "builds/" .. os.get() .. "/" )
	includedirs {"include/"}

	-- Libgit2 stuff. Assumes libgit2 was compiled normally ()
	includedirs {"libgit2/include"}

	buildoptions "-std=c++11 -stdlib=libc++"
	
	targetname ("gmsv_luagit")
	if os.is("windows") then
		targetsuffix ("_win32")
		links {"../libgit2/build/Debug/git2"}
	elseif os.is("linux") then
		targetsuffix ("_linux")
		links {"libgit2/build/git2"}
	end

	configurations { "Release" }
	
	configuration "Release"
		defines { "NDEBUG" }
		flags{ "Optimize", "FloatFast" }
	
	project "gmsv_git"
		defines {"GMMODULE"}
		files {"src/**.*"}
		kind "SharedLib"