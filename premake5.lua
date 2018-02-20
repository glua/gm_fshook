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
		if (os.target() == "windows") then
			links "Shlwapi" -- PathCanonicalize
		end
		files {
			"src/*.h",
			"src/*.hpp",
			"src/*.hxx",
			"src/*.cpp",
			"src/*.cxx"
		}
		kind "SharedLib"
