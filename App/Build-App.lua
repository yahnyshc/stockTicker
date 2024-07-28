project "App"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { "Source/**.h", "Source/**.cpp" }

   includedirs
   {
      "Source",
      -- Include Core
      "../Core/Source",
      -- "../rpi-rgb-led-matrix/include"  -- Include RGB library headers
   }
   
   libdirs { 
      -- "../rpi-rgb-led-matrix/lib"  -- Add RGB library path
   }

   links {  
      "Core", "crypto", "ssl", "cpprest", 
      "boost_program_options", "jsoncpp", "opencv_core", 
      "opencv_highgui", "opencv_imgproc", "opencv_imgcodecs",
      "pqxx",
      -- "rgbmatrix",  -- Link RGB library
      "rt",         -- Add rt library
      "m",          -- Add math library
      "pthread"     -- Add pthread library
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS" }
 
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
