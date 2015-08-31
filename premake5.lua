newoption
{
	trigger = 'gtest',
	description = 'root googletest source directory (defaults to ./googletest/googletest/)',
	value = 'path',
}

-- Premake bug on Mac: http://industriousone.com/topic/how-remove-flags-ldflags
premake.tools.gcc.ldflags.flags._Symbols = nil

local gtest_root = _OPTIONS['gtest'] or './googletest/googletest/'

function UnitTestConfig()
	includedirs { gtest_root .. '/include/' }
	links { 'gtest' }
	if os.get() ~= 'windows' then
		postbuildcommands { './%{cfg.buildtarget.relpath}' }
	else
		postbuildcommands { '%{cfg.buildtarget.relpath}' }
	end
end

function LifxConfig()
	includedirs { "./include/" }
	files { "./include/**.h" }
	
	if os.get() ~= "windows" then
		if os.get() == 'linux' then
			buildoptions { "-Wno-missing-field-initializers", "-std=c++11" }
		else
			buildoptions "-std=c++14"
		end
	else
		links { "ws2_32" }
	end
end

function CommonConfig()
	language "C++"
	
	configuration "Debug"
		defines { "DEBUG" }
		flags { "Symbols" }
		
	configuration "Release"
		defines { "NDEBUG" }
		flags { "Optimize" }
end

solution "lib-lifx"
	configurations { "Debug", "Release" }
	warnings "Extra"
	location "build"
	platforms { "x32", "x64" }
	objdir "build/obj/%{cfg.buildcfg}"
	
	project "lifx-cli"
		kind "ConsoleApp"
		targetname "lifx-cli"
		files { "./src/cli/*.h", "./src/cli/*.cpp" }
		dependson { "lib-lifx" }
		links { "lib-lifx" }
		includedirs { "./src/cli/" }

		if os.get() == 'macosx' then
			buildoptions
			{
				'-Wno-c++14-extensions',
			}
		end
		
		LifxConfig()
		CommonConfig()
		
	project "lib-lifx"
		kind "StaticLib"
		targetname "lifx"
		files { "./src/lib/*.cpp" }
		
		LifxConfig()
		CommonConfig()

	project "lifx-test"
		kind "ConsoleApp"
		files { "./test/*.cpp" }
		links { "lib-lifx" }

		LifxConfig()
		UnitTestConfig()
		CommonConfig()

	-- gtest project adapted from Jim Garrison's (@garrison) premake4 script
	-- http://jimgarrison.org/techblog/googletest-premake4.html
	project "gtest"
		kind "StaticLib"
		includedirs { gtest_root, gtest_root .. '/include/' }
		files { gtest_root .. '/src/gtest-all.cc', gtest_root .. '/src/gtest_main.cc' }

		if os.get() ~= 'windows' then
			defines { 'GTEST_HAS_PTHREAD=0' }
			links { 'pthread' }
		else
			defines { 'GTEST_HAS_PTHREAD=0' }
		end
		
		CommonConfig()
