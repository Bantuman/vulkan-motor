target_sources(${PROJECT_NAME} PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/file_system.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/os_file.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/os_file_system.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/path_utils.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/rbx_asset_cache.cpp"
)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
