shared_includes = {
    vulkan_sdk .. "/Include/",
    "../third_party/",
    "../third_party/VulkanMemoryAllocator",
    "../third_party/volk",
    "../third_party/spirv_reflect",
    "../third_party/tinygltf",
    "../third_party/stb",
    "../third_party/freetype2",
    "../third_party/harfbuzz",
    "../third_party/libpng16",
    "../third_party/curl",
    "../third_party/zlib",
    "../third_party/ffmpeg-2.0"
}

project "Engine"
    location "%{wks.location}/src"
    language "C++"
    cppdialect "C++20"

    targetdir("../lib/")
    targetname("%{prj.name}_%{cfg.buildcfg}")
    objdir("../temp/%{prj.name}_%{cfg.buildcfg}")

    defines {
        "USE_NET",
        "GLM_CONSTEXPR=constexpr",
        "GLM_FORCE_RADIANS",
        "GLM_FORCE_DEPTH_ZERO_TO_ONE",
        "GLFW_INCLUDE_VULKAN",
        "TINYGLTF_USE_RAPIDJSON",
        "TINYGLTF_NO_STB_IMAGE_WRITE"
    }

    files {
        "**.h",
        "**.hpp",
        "**.cpp",
        "shaders/*.vert",
        "shaders/*.frag",
        "shaders/*.comp",
        "shaders/*.geom",
        "../third_party/vkbootstrap/*.cpp",
        "../third_party/tinygltf/*.cpp",
        "../third_party/spirv_reflect/*.c",
        "../third_party/stb/*.cpp",
        "../third_party/volk/*.c"
    }
    removefiles {
        "main.cpp",
        "game/**.h",
        "game/**.cpp",
        "game/**.hpp",
        "editor/**.h",
        "editor/**.cpp",
        "editor/**.hpp",
    }

    includedirs {
       ".",
        "core/",
        "ecs/",
        "file/",
        -- External includes
        table.unpack(shared_includes)
    }

    libdirs {
        vulkan_sdk .. "/Lib/",
        "../lib/"
    }
    
    links {
        "glfw3",
        "harfbuzz",
        "png16",
        "libbz2",
        "avcodec",
        "avdevice",
        "avfilter",
        "avformat",
        "avutil",
        "bass",
        "swscale",
        "swresample",
  
    }

    filter "configurations:Debug"
        defines {"_DEBUG", "%{wks.name}_DEBUG"}
        runtime "Debug"
        symbols "on"
        links {
            "assimp-vc140-mtd",
            "VulkanMemoryAllocatord",
            "freetyped",
            "zlibd",
            "libcurl_imp"
        }
    filter "configurations:Release"
        defines {"_RELEASE", "%{wks.name}_RELEASE"}
        runtime "Release"
        optimize "on"
        links {
            "assimp-vc140-mt",
            "VulkanMemoryAllocator",
            "freetype",
            "zlib",
            "libcurl_imp"
        }
    filter "configurations:Retail"
        defines {"_RETAIL", "%{wks.name}_RETAIL"}
        runtime "Release"
        optimize "on"
        links {
            "assimp-vc140-mt",
            "VulkanMemoryAllocator",
            "freetype",
            "zlib",
            "libcurl_imp"
        }

    filter "system:windows"
        kind "StaticLib"
        staticruntime "off"
        symbols "on"
        systemversion "latest"
       -- warnings "Extra"
        sdlchecks "true"
        flags { 
		--	"FatalCompileWarnings",
			"MultiProcessorCompile"
		}
		links {
		--	"DXGI"
		}

		defines {
			"WIN32",
			"_CRT_SECURE_NO_WARNINGS", 
			"_LIB", 
			"_WIN32_WINNT=0x0601",
			"SYSTEM_WINDOWS"
		}
    
    -- todo: change so that this builds into /bin/shaders/
    filter "files:shaders/*"
        buildcommands('"'.. vulkan_sdk ..'/Bin/glslangValidator.exe" -V -o "../shaders/%{file.name}.spv" "%{file.relpath}"')
        buildoutputs "../shaders/%{file.name}.spv"

project "Editor"
    location "%{wks.location}/src"
    language "C++"
    cppdialect "C++20"

    targetdir("../lib/")
    targetname("%{prj.name}_%{cfg.buildcfg}")
    objdir("../temp/%{prj.name}_%{cfg.buildcfg}")

    defines { }

    files {
        "editor/**.h",
        "editor/**.hpp",
        "editor/**.cpp",
        "ui/**.h",
        "ui/**.hpp",
        "ui/**.cpp",
    }

    includedirs {
        ".",
        -- External includes
        table.unpack(shared_includes)
    }

    libdirs {
        vulkan_sdk .. "/Lib/",
        "../lib/"
    }
    
    links {
        "Engine",
    }

    filter "configurations:Debug"
        defines {"_DEBUG", "%{wks.name}_DEBUG"}
        runtime "Debug"
        symbols "on"
   --[[ filter "configurations:Release"
        defines {"_RELEASE", "%{wks.name}_RELEASE"}
        runtime "Release"
        optimize "on"
    filter "configurations:Retail"
        defines {"_RETAIL", "%{wks.name}_RETAIL"}
        runtime "Release"
        optimize "on"--]]

    filter "system:windows"
        kind "StaticLib"
        staticruntime "off"
        symbols "on"
        systemversion "latest"
        -- warnings "Extra"
        sdlchecks "true"
        flags { 
        --	"FatalCompileWarnings",
            "MultiProcessorCompile"
        }
        links {
        }

        defines {
            "WIN32",
            "_CRT_SECURE_NO_WARNINGS", 
            "_LIB", 
            "_WIN32_WINNT=0x0601",
            "SYSTEM_WINDOWS"
        }
    
print("Found Vulkan SDK: "..vulkan_sdk)
project "Game"
    location "%{wks.location}/src/"
    kind "ConsoleApp"--"WindowedApp" (it forces win32)
    language "C++"
    cppdialect "C++20"
   -- flags {"/SUBSYSTEM:CONSOLE"}

    targetdir("../bin/")
    debugdir("../bin/")
    targetname("%{prj.name}_%{cfg.buildcfg}")
    objdir("../temp/%{prj.name}/%{cfg.buildcfg}")
    
    files {
        "main.cpp",
        "game/**.h",
        "game/**.hpp",
        "game/**.cpp",
        --[[
        "ui/**.h",
        "ui/**.hpp",
        "ui/**.cpp",
        "editor/**.h",
        "editor/**.hpp",
        "editor/**.cpp"
        --]]
    }

    includedirs {
        ".",

        -- External includes
        table.unpack(shared_includes)
    }

    links {
        "Engine",
        --"CommonUtilities"
    }
    disablewarnings {"4201"}
    libdirs { 
        "../lib/" 
    }
    filter "configurations:Debug"
        defines {"_DEBUG", "%{wks.name}_DEBUG"}
        runtime "Debug"
        symbols "on"
        links { "Editor" }
    filter "configurations:Release"
        defines {"_RELEASE", "%{wks.name}_RELEASE"}
        runtime "Release"
        optimize "on"
    filter "configurations:Retail"
        defines {"_RETAIL", "%{wks.name}_RETAIL"}
        runtime "Release"
        optimize "on"
    
    filter "system:windows"
        symbols "on"
        systemversion "latest"
        warnings "Extra"
        flags { 
			"FatalCompileWarnings",
			"MultiProcessorCompile"
		}

		defines {
			"WIN32",
            "_CRT_SECURE_NO_WARNINGS",
			"_LIB",
			"SYSTEM_WINDOWS"
		}