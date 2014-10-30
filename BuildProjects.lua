solution "gmsv_git"

	language "C++"
	location ( os.get() .."-".. _ACTION )
	flags { "Symbols", "NoEditAndContinue", "StaticRuntime", "NoPCH", "EnableSSE" } --
	targetdir ( "lib/" .. os.get() .. "/" )
	includedirs { "include/" }

	links {"addlib/git2"}
	
	configurations
	{ 
		"Release"
	}
	
	configuration "Release"
		defines { "NDEBUG" }
		flags{ "Optimize", "FloatFast" }
	
	project "gmsv_git"
		defines { "GMMODULE" }
		files { "src/**.*", "include/**.*" }
		kind "SharedLib"
		