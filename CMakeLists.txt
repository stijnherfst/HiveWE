if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif ()

if (DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
	set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif ()

cmake_minimum_required(VERSION 3.19)
project (HiveWE CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(soil2 CONFIG REQUIRED)
find_package(stormlib CONFIG REQUIRED)
find_package(CascLib CONFIG REQUIRED)
find_package(JPEG REQUIRED)
find_package(qtadvanceddocking CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)

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
	${JPEG_INCLUDE_DIR}}
)

# libjpeg-turbo provides incomplete targets and thus need manual linking
find_library(TURBOJPEG_LIBRARY turbojpeg)

target_link_libraries(HiveWE PUBLIC
	Qt6::Gui
	Qt6::Widgets
	Qt6::OpenGL
	Qt6::OpenGLWidgets
	glm::glm
	soil2
	stormlib::stormlib
	${JPEG_LIBRARIES}
	${TURBOJPEG_LIBRARY}
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
)

if(WIN32)
	target_compile_definitions(HiveWE PUBLIC UNICODE _UNICODE)

	if (MSVC)
		# Properly sets the __cplusplus macro to the newest version. Might not be needed anymore in vsNext. Last checked 25/05/2021
		target_compile_options(HiveWE PUBLIC "/Zc:__cplusplus")
	endif()

	target_link_options(HiveWE PUBLIC /PROFILE)

	target_link_libraries(HiveWE PUBLIC CascLib::casc)
else()
	target_compile_definitions(HiveWE PUBLIC TBB_SUPPRESS_DEPRECATED_MESSAGES)

	target_compile_options(HiveWE PUBLIC "-Wno-multichar")

	target_link_libraries(HiveWE PUBLIC CascLib::casc_static)
endif()

target_compile_features(HiveWE PUBLIC cxx_std_20)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND}
	-E
	create_symlink "${PROJECT_SOURCE_DIR}/Data"
	"${CMAKE_CURRENT_BINARY_DIR}/Data"
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/Data"
)