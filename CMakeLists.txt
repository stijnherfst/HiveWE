cmake_minimum_required(VERSION 3.25)
project (HiveWE CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API aa1f7df0-828a-4fcd-9afc-2dc80491aca7)
set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
set(CMAKE_CXX_STANDARD 23)

find_package(Qt6 COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(soil2 CONFIG REQUIRED)
find_package(stormlib CONFIG REQUIRED)
find_package(BZip2 REQUIRED)
find_package(CascLib CONFIG REQUIRED)
find_package(JPEG REQUIRED)
find_package(qtadvanceddocking CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(libjpeg-turbo CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(outcome CONFIG REQUIRED)
find_package(unordered_dense CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

qt_standard_project_setup()

add_executable(HiveWE 
	${QT_RESOURCES}
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
	StormLib::storm
	BZip2::BZip2
	libjpeg-turbo::jpeg
	libjpeg-turbo::turbojpeg
	ads::qtadvanceddocking
	absl::flat_hash_map
	absl::flat_hash_set
	TBB::tbb
	imgui::imgui
	outcome::hl
	unordered_dense::unordered_dense
	nlohmann_json::nlohmann_json
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

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	set(QT_VCPKG_PATH "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-windows/debug")
else()
	set(QT_VCPKG_PATH "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-windows")
endif()

file(COPY "${QT_VCPKG_PATH}/Qt6/plugins/platforms/" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/platforms")
file(COPY "${QT_VCPKG_PATH}/Qt6/plugins/styles/" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/styles")
file(COPY "${QT_VCPKG_PATH}/Qt6/plugins/imageformats/" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/imageformats")