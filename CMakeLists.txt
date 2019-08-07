cmake_minimum_required(VERSION 3.13)
project (HiveWE CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

list(INSERT CMAKE_PREFIX_PATH 0 "C:\\Qt\\5.12.2\\msvc2017_64")

find_package(Qt5 REQUIRED
	Gui
	Widgets
)

find_package(glm CONFIG REQUIRED)
find_package(soil2 CONFIG REQUIRED)
find_package(stormlib CONFIG REQUIRED)
find_package(casclib CONFIG REQUIRED)
find_package(JPEG REQUIRED)


add_executable(HiveWE 
	${QT_RESOURCES}

	Camera.cpp
	HiveWE.cpp
	HiveWE.qrc
	HiveWE.rc
	HiveWE.ui
	InputHandler.cpp
	JassTokenizer.cpp
	main.cpp
	resource.h
	TerrainUndo.cpp
	Utilities.cpp
	WindowHandler.h

	Base/Doodads.cpp
	Base/FlowLayout.cpp
	Base/GameCameras.cpp
	Base/Hierarchy.cpp
	Base/Imports.cpp
	Base/Map.cpp
	Base/MapInfo.cpp
	Base/PathingMap.cpp
	Base/QRibbon.cpp
	Base/Regions.cpp
	Base/ResourceManager.cpp
	Base/ShadowMap.cpp
	Base/Sounds.cpp
	Base/Terrain.cpp
	Base/Triggers.cpp
	Base/TriggerStrings.cpp
	Base/Units.cpp

	Base/Brush/Brush.cpp
	Base/Brush/DoodadBrush.cpp
	Base/Brush/PathingBrush.cpp
	Base/Brush/TerrainBrush.cpp

	"Base/File Formats/BLP.cpp"
	"Base/File Formats/CASC.cpp"
	"Base/File Formats/INI.cpp"
	"Base/File Formats/JSON.cpp"
	"Base/File Formats/MDX.cpp"
	"Base/File Formats/MPQ.cpp"
	"Base/File Formats/SLK.cpp"

	Base/Resources/CliffMesh.cpp
	Base/Resources/GPUTexture.cpp
	Base/Resources/GroundTexture.cpp
	Base/Resources/Shader.cpp
	Base/Resources/SkinnedMesh.cpp
	Base/Resources/StaticMesh.cpp
	Base/Resources/Texture.cpp

	Menus/DoodadPalette.cpp
	Menus/DoodadPalette.ui
	Menus/ImportManager.cpp
	Menus/ImportManager.ui
	Menus/ImportManagerEdit.cpp
	Menus/ImportManagerEdit.ui
	Menus/MapInfoEditor.cpp
	Menus/MapInfoEditor.ui
	Menus/Minimap.cpp
	Menus/Minimap.ui
	Menus/Palette.cpp
	Menus/PathingPalette.cpp
	Menus/PathingPalette.ui
	Menus/SettingsEditor.cpp
	Menus/SettingsEditor.ui
	Menus/TerrainPalette.cpp
	Menus/TerrainPalette.ui
	Menus/TilePather.cpp
	Menus/TilePather.ui
	Menus/TilePicker.cpp
	Menus/TilePicker.ui
	Menus/TileSetter.cpp
	Menus/TileSetter.ui
	Menus/TriggerEditor.cpp
	Menus/TriggerEditor.ui

	"Menus/Custom Widgets/AspectRatioPixmapLabel.cpp"
	"Menus/Custom Widgets/ColorButton.cpp"
	"Menus/Custom Widgets/GLWidget.cpp"
	"Menus/Custom Widgets/JassEditor.cpp"
	"Menus/Custom Widgets/MainRibbon.cpp"
)

target_include_directories(HiveWE PRIVATE ${QSCINTILLA_INCLUDE_DIRS})
target_include_directories(HiveWE PRIVATE ${JPEG_INCLUDE_DIR}})

target_include_directories(HiveWE PRIVATE "Base" "Base/Brush" "Base/File Formats" "Base/Resources" "Menus" "Menus/Custom Widgets")

target_link_libraries(HiveWE
	Qt5::Gui
	Qt5::Widgets
	glm
	soil2
	stormlib
	casclib
	"C:/vcpkg/packages/libjpeg-turbo_x64-windows/debug/lib/turbojpegd.lib"
	"C:/vcpkg/packages/qscintilla_x64-windows/debug/lib/qscintilla2.lib"
	${JPEG_LIBRARIES}
)

if(WIN32)
    target_compile_definitions(HiveWE PUBLIC UNICODE _UNICODE)
endif()

target_compile_features(HiveWE PUBLIC cxx_std_20)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND}
	-E
	create_symlink "${PROJECT_SOURCE_DIR}/Data"
	"${CMAKE_CURRENT_BINARY_DIR}/Data"
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/Data"
)