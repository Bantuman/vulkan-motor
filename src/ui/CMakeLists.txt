target_sources(${PROJECT_NAME} PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/ui_renderer.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/game_ui.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/rich_text.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/text_info_stream.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/font_id.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/gui_object.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/gui_image_emitter.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/image_gui_object.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/text_gui_object.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/frame.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/gui_button.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/text_button.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/image_button.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/text_label.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/text_box.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/scrolling_frame.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/screen_gui.cpp"
)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
