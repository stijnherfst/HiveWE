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
	Camera.h
	HiveWE.cpp
	HiveWE.h
	HiveWE.qrc
	HiveWE.rc
	InputHandler.cpp
	InputHandler.h
	JassTokenizer.cpp
	JassTokenizer.h
	main.cpp
	QuadTree.h
	resource.h
	TerrainUndo.cpp
	TerrainUndo.h
	Utilities.cpp
	utilities.h
	WindowHandler.h

	Base/BinaryReader.h
	Base/BinaryWriter.h
	Base/Doodads.cpp
	Base/Doodads.h
	Base/FlowLayout.cpp
	Base/FlowLayout.h
	Base/GameCameras.cpp
	Base/GameCameras.h
	Base/Hierarchy.cpp
	Base/Hierarchy.h
	Base/Imports.cpp
	Base/Imports.h
	Base/Map.cpp
	Base/Map.h
	Base/MapInfo.cpp
	Base/MapInfo.h
	Base/PathingMap.cpp
	Base/PathingMap.h
	Base/QRibbon.cpp
	Base/QRibbon.h
	Base/Regions.cpp
	Base/Regions.h
	Base/ResourceManager.cpp
	Base/ResourceManager.h
	Base/ShadowMap.cpp
	Base/ShadowMap.h
	Base/Sounds.cpp
	Base/Sounds.h
	Base/Terrain.cpp
	Base/Terrain.h
	Base/Triggers.cpp
	Base/Triggers.h
	Base/TriggerStrings.cpp
	Base/TriggerStrings.h
	Base/Units.cpp
	Base/Units.h

	Base/Brush/Brush.cpp
	Base/Brush/Brush.h
	Base/Brush/DoodadBrush.cpp
	Base/Brush/DoodadBrush.h
	Base/Brush/PathingBrush.cpp
	Base/Brush/PathingBrush.h
	Base/Brush/TerrainBrush.cpp
	Base/Brush/TerrainBrush.h

	"Base/File Formats/BLP.cpp"
	"Base/File Formats/BLP.h"
	"Base/File Formats/CASC.cpp"
	"Base/File Formats/CASC.h"
	"Base/File Formats/INI.cpp"
	"Base/File Formats/INI.h"
	"Base/File Formats/JSON.cpp"
	"Base/File Formats/JSON.h"
	"Base/File Formats/MDX.cpp"
	"Base/File Formats/MDX.h"
	"Base/File Formats/MPQ.cpp"
	"Base/File Formats/MPQ.h"
	"Base/File Formats/SLK.cpp"
	"Base/File Formats/SLK.h"

	Base/Resources/CliffMesh.cpp
	Base/Resources/CliffMesh.h
	Base/Resources/GPUTexture.cpp
	Base/Resources/GPUTexture.h
	Base/Resources/GroundTexture.cpp
	Base/Resources/GroundTexture.h
	Base/Resources/Shader.cpp
	Base/Resources/Shader.h
	Base/Resources/SkinnedMesh.cpp
	Base/Resources/SkinnedMesh.h
	Base/Resources/StaticMesh.cpp
	Base/Resources/StaticMesh.h
	Base/Resources/Texture.cpp
	Base/Resources/Texture.h

	Menus/DoodadPalette.cpp
	Menus/DoodadPalette.h
	Menus/DoodadPalette.ui
	Menus/ImportManager.cpp
	Menus/ImportManager.h
	Menus/ImportManager.ui
	Menus/ImportManagerEdit.cpp
	Menus/ImportManagerEdit.h
	Menus/ImportManagerEdit.ui
	Menus/MapInfoEditor.cpp
	Menus/MapInfoEditor.h
	Menus/MapInfoEditor.ui
	Menus/Minimap.cpp
	Menus/Minimap.h
	Menus/Minimap.ui
	Menus/Palette.cpp
	Menus/Palette.h
	Menus/PathingPalette.cpp
	Menus/PathingPalette.h
	Menus/PathingPalette.ui
	Menus/SettingsEditor.cpp
	Menus/SettingsEditor.h
	Menus/SettingsEditor.ui
	Menus/TerrainPalette.cpp
	Menus/TerrainPalette.h
	Menus/TerrainPalette.ui
	Menus/TilePather.cpp
	Menus/TilePather.h
	Menus/TilePather.ui
	Menus/TilePicker.cpp
	Menus/TilePicker.h
	Menus/TilePicker.ui
	Menus/TileSetter.cpp
	Menus/TileSetter.h
	Menus/TileSetter.ui
	Menus/TriggerEditor.cpp
	Menus/TriggerEditor.h
	Menus/TriggerEditor.ui

	"Menus/Custom Widgets/AspectRatioPixmapLabel.cpp"
	"Menus/Custom Widgets/AspectRatioPixmapLabel.h"
	"Menus/Custom Widgets/ColorButton.cpp"
	"Menus/Custom Widgets/ColorButton.h"
	"Menus/Custom Widgets/GLWidget.cpp"
	"Menus/Custom Widgets/GLWidget.h"
	"Menus/Custom Widgets/JassEditor.cpp"
	"Menus/Custom Widgets/JassEditor.h"
	"Menus/Custom Widgets/MainRibbon.cpp"
	"Menus/Custom Widgets/MainRibbon.h"
)

target_include_directories(HiveWE PRIVATE ${QSCINTILLA_INCLUDE_DIRS})
target_include_directories(HiveWE PRIVATE ${JPEG_INCLUDE_DIR}})
# target_include_directories(HiveWE PRIVATE ${CASCLIB_INCLUDE_DIRS})
# target_include_directories(HiveWE PRIVATE ${STORMLIB_INCLUDE_DIRS})

target_include_directories(HiveWE PRIVATE "Base" "Base/Brush" "Base/File Formats" "Base/Resources" "Menus" "Menus/Custom Widgets")

message("Ayyyyyyyyy")
message(${JPEG_LIBRARIES})
message("Ayyyyyyyyy")
message(${JPEG_INCLUDE_DIR}})
message("Ayyyyyyyyy")


target_link_libraries(HiveWE
	Qt5::Gui
	Qt5::Widgets
	glm
	soil2
	stormlib
	casclib
	${JPEG_LIBRARIES}
	${QSCINTILLA_LIBRARIES}
)

if(WIN32)
    target_compile_definitions(HiveWE PRIVATE UNICODE _UNICODE)
endif()

target_compile_features(HiveWE PUBLIC cxx_std_20)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND}
	-E
	create_symlink "${PROJECT_SOURCE_DIR}/Data"
	"${CMAKE_CURRENT_BINARY_DIR}/Data"
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/Data"
)