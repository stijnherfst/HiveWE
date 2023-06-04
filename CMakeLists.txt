cmake_minimum_required(VERSION 3.26)
project (HiveWE CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API 2182bf5c-ef0d-489a-91da-49dbc3090d2a)
set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
set(CMAKE_CXX_STANDARD 23)

find_package(Qt6 COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(soil2 CONFIG REQUIRED)
find_package(stormlib CONFIG REQUIRED)
find_package(CascLib CONFIG REQUIRED)
find_package(JPEG REQUIRED)
find_package(qtadvanceddocking CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(libjpeg-turbo CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(outcome CONFIG REQUIRED)

qt_standard_project_setup()

add_executable(HiveWE 
	${QT_RESOURCES}
)

file(COPY
  # you don't want to hardcode the path here in a real-world project,
  # but you get the idea
  "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.35.32215/modules/std.ixx"
  DESTINATION
  ${PROJECT_SOURCE_DIR}/src/
)

add_subdirectory(src)

target_include_directories(HiveWE PRIVATE 
	"src" 
	"src/base" 
	"src/brush" 
	"src/custom_widgets" 
	"src/file_formats" 
	"src/main_window" 
	"src/menus" 
	"src/models"
	"src/model_editor"
	"src/object_editor"
	"src/resources" 
	"src/resources/skinned_mesh" 
	"src/utilities"

	${BULLET_INCLUDE_DIR}
)

target_link_libraries(HiveWE PUBLIC
	Qt6::Gui
	Qt6::Widgets
	Qt6::OpenGL
	Qt6::OpenGLWidgets
	glm::glm
	glad::glad
	soil2::soil2
	stormlib::stormlib
	libjpeg-turbo::jpeg
	libjpeg-turbo::turbojpeg
	ads::qtadvanceddocking
	absl::flat_hash_map
	absl::flat_hash_set
	fmt::fmt
	fmt::fmt-header-only
	TBB::tbb
	imgui::imgui
	outcome::hl
	${BULLET_LIBRARIES}
)

target_compile_options(HiveWE PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /MP /sdl /diagnostics:caret>
	$<$<CXX_COMPILER_ID:Clang>:-Wextra -Wpedantic -Werror -Wno-multichar>
	$<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic -Werror -Wno-multichar>
)

target_compile_definitions(HiveWE PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:UNICODE _UNICODE>
	$<$<CXX_COMPILER_ID:Clang>:TBB_SUPPRESS_DEPRECATED_MESSAGES>
	$<$<CXX_COMPILER_ID:GNU>:TBB_SUPPRESS_DEPRECATED_MESSAGES>
)

if(WIN32)
	target_link_libraries(HiveWE PUBLIC CascLib::casc)
else()
	target_link_libraries(HiveWE PUBLIC CascLib::casc_static)
endif()

target_compile_features(HiveWE PUBLIC cxx_std_23)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND}
	-E
	create_symlink "${PROJECT_SOURCE_DIR}/data"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data"
	DEPENDS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data"
)

message(${CMAKE_BUILD_TYPE})
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	file(COPY "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-windows/debug/Qt6/plugins/platforms/"
			DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/platforms/"
	)
else()
	file(COPY "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-windows/Qt6/plugins/platforms/"
			DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/platforms/"
	)
endif()