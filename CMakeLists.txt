if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
			CACHE STRING "")
endif ()
if (DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
	set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}"
			CACHE STRING "")
endif ()

cmake_minimum_required(VERSION 3.13)
project (HiveWE CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED
	Gui
	Widgets
)
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
	HiveWE.rc
	Camera.cpp
	HiveWE.cpp
	InputHandler.cpp
	JassTokenizer.cpp
	main.cpp
	TerrainUndo.cpp
	Utilities.cpp
	WindowHandler.h

	"test.cpp"

	Base/Doodads.cpp
	Base/FlowLayout.cpp
	Base/GameCameras.cpp
	Base/Hierarchy.cpp
	Base/Imports.cpp
	Base/Map.cpp
	Base/MapInfo.cpp
	Base/PathingMap.cpp
	Base/Regions.cpp
	
	Base/RenderManager.cpp
	Base/ShadowMap.cpp
	Base/Sounds.cpp
	Base/Terrain.cpp
	Base/Triggers.cpp
	Base/TriggerStrings.cpp
	Base/Units.cpp
	Base/Physics.cpp

	Base/Brush/Brush.cpp
	Base/Brush/DoodadBrush.cpp
	Base/Brush/PathingBrush.cpp
	Base/Brush/TerrainBrush.cpp
	Base/Brush/UnitBrush.cpp

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
	Base/Resources/PathingTexture.cpp
	Base/Resources/QIconResource.cpp

	Base/Resources/SkinnedMesh/RenderNode.cpp
	Base/Resources/SkinnedMesh/SkeletalModelInstance.cpp

	Menus/DoodadPalette.cpp
	Menus/ImportManager.cpp
	Menus/ImportManagerEdit.cpp
	Menus/MapInfoEditor.cpp
	Menus/Minimap.cpp
	Menus/Palette.cpp
	Menus/PathingPalette.cpp
	Menus/SettingsEditor.cpp
	Menus/TerrainPalette.cpp
	Menus/UnitPalette.cpp
	Menus/TilePather.cpp
	Menus/TilePicker.cpp
	Menus/TileSetter.cpp
	Menus/TriggerEditor.cpp
	Menus/Selections.cpp
	Menus/ObjectEditor.cpp

	Base/Models/SingleModel.cpp
	Base/Models/TableModel.cpp
	Base/Models/BaseTreeModel.cpp
	Base/Models/UnitTreeModel.cpp
	Base/Models/DoodadTreeModel.cpp
	Base/Models/ItemTreeModel.cpp
	Base/Models/AbilityTreeModel.cpp
	Base/Models/BuffTreeModel.cpp
	Base/Models/UpgradeTreeModel.cpp
	Base/Models/UnitListModel.cpp
	Base/Models/DoodadListModel.cpp
	Base/Models/DestructableListModel.cpp
	Base/Models/DestructibleTreeModel.cpp
	Base/Models/DestructibleTreeModel.h

	"Menus/Custom Widgets/AspectRatioPixmapLabel.cpp"
	"Menus/Custom Widgets/ColorButton.cpp"
	"Menus/Custom Widgets/GLWidget.cpp"
	"Menus/Custom Widgets/JassEditor.cpp"
	"Menus/Custom Widgets/QRibbon.cpp"
	"Menus/Custom Widgets/MainRibbon.cpp"
	"Menus/Custom Widgets/SearchWindow.cpp"
	"Menus/Custom Widgets/VariableEditor.cpp"
	"Menus/Custom Widgets/TriggerExplorer.cpp"
	"Menus/Custom Widgets/UnitSelector.cpp"
	"Menus/Custom Widgets/GenericSelectorList.cpp"
	"Menus/Custom Widgets/IconView.cpp"

	#tracy/TracyClient.cpp
 )

target_include_directories(HiveWE PRIVATE ${JPEG_INCLUDE_DIR}})
target_include_directories(HiveWE PRIVATE ${BULLET_INCLUDE_DIR})
target_include_directories(HiveWE PRIVATE "Base" "Base/Brush" "Base/File Formats" "Base/Resources" "Base/Resources/SkinnedMesh" "Menus" "Menus/Custom Widgets" "Base/Models")

target_link_libraries(HiveWE
	Qt5::Gui
	Qt5::Widgets
	glm
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
	fmt::fmt
	fmt::fmt-header-only
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux") #Sadly only dynamic triplets work because of casclib and stormlib
	target_link_libraries(HiveWE
		"$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib/libqscintilla2_qt5.so"
		"$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib/libturbojpeg.so"
	)
endif()
if(WIN32)
	if (CMAKE_BUILD_TYPE STREQUAL "Release")
		target_link_libraries(HiveWE
			"$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib/turbojpeg.lib"
			"$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib/qscintilla2_qt5.lib")
	else()
		target_link_libraries(HiveWE
			"$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib/turbojpeg.lib"
			"$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/debug/lib/qscintilla2_qt5d.lib")
	endif()
	#target_compile_definitions(HiveWE PUBLIC UNICODE _UNICODE _HAS_DEPRECATED_RESULT_OF TRACY_ENABLE)
	target_compile_definitions(HiveWE PUBLIC UNICODE _UNICODE _HAS_DEPRECATED_RESULT_OF)
	target_link_options(HiveWE PUBLIC /PROFILE)
endif()
target_compile_features(HiveWE PUBLIC cxx_std_20)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND}
	-E
	create_symlink "${PROJECT_SOURCE_DIR}/Data"
	"${CMAKE_CURRENT_BINARY_DIR}/Data"
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/Data"
)
