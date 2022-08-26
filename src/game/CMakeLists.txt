target_sources(${PROJECT_NAME} PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/instance.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/data_model.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/gameworld.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/ambience.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/sky.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/camera.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/game_renderer.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/instance_factory.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/instance_utils.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/context_action.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/ancestry_changed_callbacks.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/destroyed_callbacks.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/geom_renderer.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/decal_renderer.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/skybox_renderer.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/geom.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/decal.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/mesh_geom.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/rigged_mesh_loader.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/rig_component.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/animator.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/attachment.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/bone_attachment.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/model.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/profiler_frontend.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/editor_camera.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/game_camera.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/player.cpp"
)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(ui)
