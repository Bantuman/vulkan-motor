target_sources(${PROJECT_NAME} PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/logging.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/profiler.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/application.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/utf8.cpp"
)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")


