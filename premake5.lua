function CommonConfig()
	language "C++"
	
	if os.get() ~= "windows" then
		buildoptions "-std=c++11"
	else
		links { "ws2_32" }
	end
	
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
		files { "./include/**.h", "./src/cli/*.h", "./src/cli/*.cpp" }
		dependson { "lib-lifx" }
		links { "lib-lifx" }
		includedirs { "./include/", "./src/cli/" }
		
		CommonConfig()
		
	project "lib-lifx"
		kind "StaticLib"
		targetname "liblifx"
		files { "./src/lib/*.cpp" }
		includedirs { "./include/" }
		
		CommonConfig()
