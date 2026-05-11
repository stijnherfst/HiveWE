cmake_minimum_required(VERSION 3.29)

# Hack because vcpkg doesn't allow you to just declare that you want to be static on all platforms
# Linux and MacOS are static by default
# if(WIN32)
#    SET(VCPKG_TARGET_TRIPLET "x64-windows-static")
# endif()

project (HiveWE CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(soil2 CONFIG REQUIRED)
find_package(stormlib CONFIG REQUIRED)
find_package(BZip2 REQUIRED)
find_package(CascLib CONFIG REQUIRED)
find_package(JPEG REQUIRED)
find_package(qtadvanceddocking-qt6 CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(libjpeg-turbo CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(outcome CONFIG REQUIRED)
find_package(unordered_dense CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(mimalloc CONFIG REQUIRED)
find_package(unofficial-qscintilla CONFIG REQUIRED)
find_package(tracy CONFIG REQUIRED)

qt_standard_project_setup()

option(HIVEWE_BUILD_TESTS "Build HiveWE unit tests" ON)

add_library(HiveWE_lib STATIC)
add_executable(HiveWE
	${QT_RESOURCES}
)

add_subdirectory(src)

target_include_directories(HiveWE_lib PUBLIC
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
	"src/trigger_editor"
	"src/resources"
	"src/resources/skinned_mesh"
	"src/utilities"

	${BULLET_INCLUDE_DIR}
)

target_link_libraries(HiveWE_lib PUBLIC
	mimalloc
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
	absl::base
	libjpeg-turbo::turbojpeg
	ads::qtadvanceddocking-qt6
	TBB::tbb
	imgui::imgui
	outcome::hl
	unordered_dense::unordered_dense
	nlohmann_json::nlohmann_json
	unofficial::qscintilla::qscintilla
	${BULLET_LIBRARIES}
	Tracy::TracyClient
)

target_compile_options(HiveWE_lib PUBLIC
	$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /MP /sdl /diagnostics:caret /permissive->
	$<$<CXX_COMPILER_ID:Clang>:-Wextra -Wpedantic -Werror -Wno-multichar>
	$<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic -Werror -Wno-multichar>
)

target_compile_definitions(HiveWE_lib PUBLIC
	$<$<CXX_COMPILER_ID:MSVC>:UNICODE _UNICODE>
	$<$<CXX_COMPILER_ID:Clang>:TBB_SUPPRESS_DEPRECATED_MESSAGES>
	$<$<CXX_COMPILER_ID:GNU>:TBB_SUPPRESS_DEPRECATED_MESSAGES>
)

if(TRACY_ENABLE)
	target_compile_definitions(HiveWE_lib PUBLIC TRACY_ENABLE)
endif()

if(TRACY_NO_EXIT)
	target_compile_definitions(HiveWE_lib PUBLIC TRACY_NO_EXIT)
endif()

if(WIN32)
	target_link_libraries(HiveWE_lib PUBLIC CascLib::casc)
else()
	target_link_libraries(HiveWE_lib PUBLIC CascLib::casc_static)
endif()

target_compile_features(HiveWE_lib PUBLIC cxx_std_23)

target_link_libraries(HiveWE PRIVATE HiveWE_lib)

if(HIVEWE_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND}
	-E create_symlink
	"${PROJECT_SOURCE_DIR}/data"
	"$<TARGET_FILE_DIR:HiveWE>/data"
)

# vcpkg's tracy port installs the Debug DLL under debug/bin/Debug/ (extra
# subfolder), which VCPKG_APPLOCAL_DEPS doesn't pick up. Copy it manually using
# the imported target's known location.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"$<TARGET_FILE:Tracy::TracyClient>"
	"$<TARGET_FILE_DIR:HiveWE>"
)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
	set(QT_VCPKG_PATH "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-windows/debug")
else()
	set(QT_VCPKG_PATH "${CMAKE_CURRENT_BINARY_DIR}/vcpkg_installed/x64-windows")
endif()

file(COPY "${QT_VCPKG_PATH}/Qt6/plugins/platforms/" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/platforms")
file(COPY "${QT_VCPKG_PATH}/Qt6/plugins/styles/" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/styles")
file(COPY "${QT_VCPKG_PATH}/Qt6/plugins/imageformats/" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/imageformats")