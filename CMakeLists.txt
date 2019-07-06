cmake_minimum_required(VERSION 3.13)
project (HiveWE CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

list(INSERT CMAKE_PREFIX_PATH 0 "C:\\Qt\\5.12.2\\msvc2017_64")

find_package(Qt5 REQUIRED  
	Qml
	Gui
	Widgets
)

find_package(soil2 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

set(HiveWE_sources
	AspectRatioPixmapLabel.cpp
	BLP.cpp
	Brush.cpp
	Camera.cpp
	CASC.cpp
	CliffMesh.cpp
	ColorButton.cpp
	DoodadBrush.cpp
	DoodadPalette.cpp
	Doodads.cpp
	FlowLayout.cpp
	GameCameras.cpp
	GLWidget.cpp
	GPUTexture.cpp
	GroundTexture.cpp
	Hierarchy.cpp
	HiveWE.cpp
	ImportManager.cpp
	ImportManagerEdit.cpp
	Imports.cpp
	INI.cpp
	InputHandler.cpp
	JassEditor.cpp
	JassTokenizer.cpp
	JSON.cpp
	main.cpp
	MainRibbon.cpp
	Map.cpp
	MapInfo.cpp
	MapInfoEditor.cpp
	MDX.cpp
	Minimap.cpp
	MPQ.cpp
	Palette.cpp
	PathingBrush.cpp
	PathingMap.cpp
	PathingPallete.cpp
	QRibbon.cpp
	Quadtree.cpp
	Regions.cpp
	ResourceManager.cpp
	SettingsEditor.cpp
	Shader.cpp
	ShadowMap.cpp
	SkinnedMesh.cpp
	SLK.cpp
	Sounds.cpp
	StaticMesh.cpp
	stdafx.cpp
	Terrain.cpp
	TerrainBrush.cpp
	TerrainPalette.cpp
	TerrainUndo.cpp
	Texture.cpp
	TilePather.cpp
	TilePicker.cpp
	TileSetter.cpp
	TriggerEditor.cpp
	Triggers.cpp
	TriggerStrings.cpp
	Units.cpp
	Utilities.cpp
	WindowHandler.cpp
)

add_executable(HiveWE ${HiveWE_sources} ${QT_RESOURCES})

target_include_directories(HiveWE PRIVATE ${QSCINTILLA_INCLUDE_DIRS})
target_include_directories(HiveWE PRIVATE ${LIBJPEGTURBO_INCLUDE_DIRS})
target_include_directories(HiveWE PRIVATE ${CASCLIB_INCLUDE_DIRS})
target_include_directories(HiveWE PRIVATE ${STORMLIB_INCLUDE_DIRS})

target_link_libraries(HiveWE
	Qt5::Gui
	Qt5::Widgets
	glm
	casclib
	stormlib
	libjpeg
	qscintilla
	soil2
)


target_compile_features(HiveWE PUBLIC cxx_std_20)

if (MSVC)
   set_target_properties(HiveWE PROPERTIES COMPILE_FLAGS "/Yustdafx.h")
   set_source_files_properties(HiveWE_sources PROPERTIES COMPILE_FLAGS "/Ycstdafx.h")
   set_source_files_properties("mocs_compilation.cpp" PROPERTIES COMPILE_FLAGS "/Y-")
endif(MSVC)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND}
	-E
	create_symlink "${PROJECT_SOURCE_DIR}/Data"
	"${CMAKE_CURRENT_BINARY_DIR}/Data"
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/Data"
)