if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif ()

if (DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
	set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif ()

cmake_minimum_required(VERSION 3.13)
project (HiveWE CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set (DVCPKG_CHAINLOAD_TOOLCHAIN_FILE "C:/Qt/5.15.0/msvc2019_64")

find_package(Qt5 REQUIRED Gui Widgets)
if(NOT DEFINED WIN32)
find_package(Qt5 REQUIRED X11Extras)
endif()
find_package(Bullet CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(soil2 CONFIG REQUIRED)
find_package(stormlib CONFIG REQUIRED)
find_package(casclib CONFIG REQUIRED)
find_package(JPEG REQUIRED)
find_package(qtadvanceddocking CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_executable(HiveWE 
	${QT_RESOURCES}
 )

add_subdirectory(src)

target_include_directories(HiveWE PRIVATE 
	"src" 
	"src/Base" 
	"src/Brush" 
	"src/File Formats" 
	"src/Resources" 
	"src/Resources/SkinnedMesh" 
	"src/Menus" 
	"src/Custom Widgets" 
	"src/Models"

	${BULLET_INCLUDE_DIR}
	${JPEG_INCLUDE_DIR}}
)

target_link_libraries(HiveWE
	Qt5::Gui
	Qt5::Widgets
	glm::glm
	soil2
	stormlib::stormlib
	casclib
	${JPEG_LIBRARIES}
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
)
if(NOT DEFINED WIN32)
	target_link_libraries(HiveWE Qt5::X11Extras)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux") # Sadly only dynamic triplets work because of casclib and stormlib
	find_library(QSCINTILLA_LIBRARY qscintilla2_qt5)
	find_library(TURBOJPEG_LIBRARY turbojpeg)
endif()

if(WIN32)
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		find_library(QSCINTILLA_LIBRARY qscintilla2_qt5d)
		find_library(TURBOJPEG_LIBRARY turbojpegd)
	else()
		find_library(QSCINTILLA_LIBRARY qscintilla2_qt5)
		find_library(TURBOJPEG_LIBRARY turbojpeg)
	endif()

	target_compile_definitions(HiveWE PUBLIC UNICODE _UNICODE)

	if (MSVC)
		# Properly sets the __cplusplus macro to the newest version. Might not be needed anymore in vsNext. Last checked 25/05/2021
		target_compile_options(HiveWE PUBLIC "/Zc:__cplusplus")
	endif()

	target_link_options(HiveWE PUBLIC /PROFILE)
endif()

# libjpeg-turbo and qscintilla provide incomplete targets and thus need manual linking
target_link_libraries(HiveWE
	${QSCINTILLA_LIBRARY}
	${TURBOJPEG_LIBRARY}
)

target_compile_features(HiveWE PUBLIC cxx_std_20)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND}
	-E
	create_symlink "${PROJECT_SOURCE_DIR}/Data"
	"${CMAKE_CURRENT_BINARY_DIR}/Data"
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/Data"
)
