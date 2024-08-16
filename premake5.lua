workspace "Limited86Machine"
	configurations { "Debug", "Release" }
	location "build"

project "Limited86Machine"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	targetdir "bin/%{cfg.buildcfg}"

	files { "**.h", "**.cpp", "dtest/**.cpp", "dtest/**.h" }

	filter "configurations:Debug"
	defines { "DEBUG" }
    symbols "On"

