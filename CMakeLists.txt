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

add_subdirectory(src)

get_property(target_files GLOBAL PROPERTY "target_files" ) 

file(GLOB_RECURSE project_headers src/*.h)
file(GLOB_RECURSE project_headers2 "src/Menus/Custom Widgets/*.h")
foreach(sgbd_file ${project_headers2})
	message(${sgbd_file})
	list(REMOVE_ITEM project_headers ${sgbd_file})
endforeach(sgbd_file)

list(APPEND target_files ${project_headers})

list(REMOVE_DUPLICATES target_files)

add_executable(HiveWE
	${target_files}
	${QT_RESOURCES}
)

set(src_path ${PROJECT_SOURCE_DIR})

message(${src_path})

foreach(sgbd_file ${target_files})
	string(REGEX REPLACE ${src_path}/ ""  sgbd_fpath ${sgbd_file})
	string(REGEX REPLACE  "\(.*\)/.*" \\1  sgbd_fpath2 ${sgbd_fpath})
	string(REGEX REPLACE  "src" "source files"  sgbd_fpath3 ${sgbd_fpath2})
	source_group(${sgbd_fpath3} FILES ${sgbd_file})
endforeach(sgbd_file)

 
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

	# _HAS_DEPRECATED_RESULT_OF is because of abseil using deprecated functionality. Last checked at 10-03-2021
	# _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING is because of abseil using deprecated functionality. Last checked at 10-03-2021
	target_compile_definitions(HiveWE PUBLIC UNICODE _UNICODE _HAS_DEPRECATED_RESULT_OF _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING)

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
