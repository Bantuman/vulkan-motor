include "premake/extensions.lua"

workspace "LMLE"
	location "."
	startproject "Game"
	architecture "x64"

	configurations {
		"Debug",
		"Release",
		"Retail"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
vulkan_sdk = os.getenv("VULKAN_SDK") or "D:/SDK/Vulkan SDK 1-3-216-0"

include "src"