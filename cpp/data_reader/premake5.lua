
project "reader"
    kind "ConsoleApp"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/obj/" .. outputdir .. "/%{prj.name}")

    files 
    { 
        "src/**.h", 
        "src/**.hpp", 
        "src/**.c", 
        "src/**.cpp",
        "%{wks.location}/utils/**.h",
        "%{wks.location}/utils/**.hpp",
    }

    includedirs
    {
        ".",
        "/usr/include/SDL2",
    }

    libdirs
    {
        "/usr/lib/x86_64-linux-gnu",
    }

    links
    {
        "util",
        "SDL2",
        "SDL2_ttf",
        "X11",
        "rt",
    }

    filter "configurations:Debug"
    defines { "DEBUG" }
    symbols "On"

    filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "On"

