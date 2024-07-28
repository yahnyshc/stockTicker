project "Core"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { "Source/**.cpp", "Source/**.hpp" }

   includedirs
   {
      "Source",
      "/usr/include/boost",
      "/usr/include/jsoncpp",
      "/usr/include/openssl",
      "/usr/include/opencv4",
      -- "../rpi-rgb-led-matrix/include"  -- Include RGB library headers
   }
   
   libdirs { 
      -- "../rpi-rgb-led-matrix/lib"  -- Add RGB library path
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"
