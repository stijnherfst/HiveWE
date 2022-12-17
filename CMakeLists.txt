if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif ()

cmake_minimum_required(VERSION 3.22)
project (HiveWE CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

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

add_subdirectory(src)

target_include_directories(HiveWE PRIVATE 
	"src" 
	"src/Base" 
	"src/Brush" 
	"src/Custom Widgets" 
	"src/File Formats" 
	"src/Menus" 
	"src/Models"
	"src/Resources" 
	"src/Resources/SkinnedMesh" 
	"src/Utilities"

	${BULLET_INCLUDE_DIR}
)

target_link_libraries(HiveWE PUBLIC
	Qt6::Gui
	Qt6::Widgets
	Qt6::OpenGL
	Qt6::OpenGLWidgets
	glm::glm
	glad::glad
	soil2
	stormlib::stormlib
	libjpeg-turbo::jpeg
	libjpeg-turbo::turbojpeg
	ads::qtadvanceddocking
	Bullet3Common
	BulletDynamics
	BulletSoftBody
	BulletInverseDynamics
	BulletCollision
	LinearMath
	absl::flat_hash_map
	absl::flat_hash_set
	fmt::fmt
	fmt::fmt-header-only
	TBB::tbb
	imgui::imgui
	outcome::hl
)

target_compile_options(HiveWE PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /MP /W4 /sdl>
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


message("${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND}
	-E
	create_symlink "${PROJECT_SOURCE_DIR}/Data"
	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Data"
	DEPENDS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Data"
)

#install(TARGETS HiveWE
#    BUNDLE  DESTINATION .
#    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#)

#qt_generate_deploy_app_script(
 #   TARGET HiveWE
#    FILENAME_VARIABLE deploy_script
#    NO_UNSUPPORTED_PLATFORM_ERROR
#)

#install(SCRIPT ${deploy_script})