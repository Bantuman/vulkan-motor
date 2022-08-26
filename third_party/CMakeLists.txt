find_package(Vulkan REQUIRED)
find_package(rapidjson REQUIRED)

add_library(spirv_reflect STATIC)
add_library(stb STATIC)
add_library(TinyGLTF STATIC)

add_subdirectory(freetype-2.12.0)

add_compile_definitions(
	TINYGLTF_USE_RAPIDJSON
	TINYGLTF_NO_STB_IMAGE_WRITE
)

target_include_directories(spirv_reflect PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/spirv_reflect"
)

target_include_directories(spirv_reflect PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/spirv_reflect/include"
)

target_sources(spirv_reflect PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/spirv_reflect/spirv_reflect.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/spirv_reflect/spirv_reflect.c"
)

target_include_directories(stb PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/stb"
)

target_sources(stb PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/stb/stb_image.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/stb/stb_truetype.cpp"
)

target_include_directories(TinyGLTF PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/tinygltf"
)

target_sources(TinyGLTF PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/tinygltf/tiny_gltf.cpp"
)

target_link_libraries(TinyGLTF PRIVATE stb)
target_link_libraries(TinyGLTF PRIVATE rapidjson)

