-- premake5.lua
workspace "data_simulator"

   location "build"

   architecture "x86_64"
   staticruntime "on"

   language "C++"
   cppdialect "C++17"

   flags { "MultiProcessorCompile" }

   configurations { "Debug", "Release" }

   filter "configurations:Debug"
      defines "DEBUG"
      optimize "Debug"
      symbols "On"

   filter "configurations:Release"
      defines "NDEBUG"
      optimize "Speed"
      symbols "Off"

   filter {}

-- used by projects in this workspace
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "data_generator"
include "data_reader"
