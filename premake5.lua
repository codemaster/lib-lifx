newoption
{
	trigger = 'gtest',
	description = 'root googletest source directory (defaults to ./googletest/googletest/)',
	value = 'path',
}

local gtest_root = _OPTIONS['gtest'] or './googletest/googletest/'

function UnitTestConfig()
	includedirs { gtest_root .. '/include/' }
	links { 'gtest' }
end

function LifxConfig()
	includedirs { "./include/" }
	files { "./include/**.h" }
	
	if os.get() ~= "windows" then
		if os.get() == 'linux' then
			buildoptions "-Wno-missing-field-initializers"
		end
		buildoptions "-std=c++14"
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
		language "C++"
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
