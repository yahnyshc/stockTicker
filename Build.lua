-- premake5.lua
workspace "New Project"
   configurations { "Debug", "Release", "Dist" }
   startproject "App"

   -- Detect architecture based on platform
   if os.host() == "linux" then
      local arch = io.popen("uname -m"):read("*l")
      if arch == "armv7l" or arch == "aarch64" then
         architecture "ARM"
      else
         architecture "x64"  -- Default to x64 for other architectures
      end
   else
      architecture "x64"  -- Default to x64 for non-Linux platforms
   end

   -- Workspace-wide build options for MSVC
   filter "system:windows"
      buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

group "Core"
	include "Core/Build-Core.lua"
group ""

include "App/Build-App.lua"
