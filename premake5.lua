solution "gm_fs"

	language "C++"
	location "project"
	flags { "StaticRuntime", "C++11" }
	targetdir "bin"
	architecture "x86"

	configurations { "Release" }

	configuration "Release"
		optimize "On"

	project "gm_fs"
		characterset "MBCS"
		include "LuaInterface"
		include "SourceSDK"
		include "SourceSDK/Tier0"
		include "SourceSDK/Tier1"
		include "SourceSDK/Tier2"
		files {
			"src/*.h",
			"src/*.hpp",
			"src/*.hxx",
			"src/*.cpp",
			"src/*.cxx"
		}
		kind "SharedLib"
		filter "system:windows"
			defines "_DLL_EXT=dll"
		filter "system:linux"
			defines "_DLL_EXT=so"
		filter "system:macosx"
			defines "_DLL_EXT=dylib"
