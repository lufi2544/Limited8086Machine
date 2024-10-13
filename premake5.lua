workspace "Limited86Machine"
    configurations { "Debug", "Release" }
    location "build"
    platforms { "x64" }

    filter "system:windows"
        toolset "clang"  -- Use MSVC on Windows

project "Limited86Machine"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    targetdir "bin/%{cfg.buildcfg}"
	symbols "On"

    files { "source/**.h", "source/**.cpp", "dtest/**.cpp", "dtest/**.h" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"

