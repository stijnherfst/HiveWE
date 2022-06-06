target_sources(HiveWE PRIVATE
	HiveWE.rc
	HiveWE.cpp
	main.cpp
	test.cpp

	Base/BinaryReader.ixx
	Base/BinaryWriter.ixx
	Base/no_init_allocator.ixx
	Base/ResourceManager.ixx
	Base/Doodads.cpp
	Base/Hierarchy.ixx
	Base/GameCameras.ixx
	Base/Imports.ixx
	Base/Map.cpp
	Base/MapInfo.ixx
	Base/PathingMap.cpp
	Base/Regions.cpp
	Base/Camera.cpp
	Base/InputHandler.cpp
	Base/TerrainUndo.cpp
	Base/WindowHandler.h
	Base/Sounds.ixx
	Base/RenderManager.cpp
	Base/ShadowMap.cpp
	Base/Terrain.cpp
	Base/Triggers.cpp
	Base/TriggerStrings.cpp
	Base/Units.cpp
	Base/Physics.cpp
	Base/MathOperations.ixx

	Brush/Brush.cpp
	Brush/DoodadBrush.cpp
	Brush/PathingBrush.cpp
	Brush/TerrainBrush.cpp
	Brush/UnitBrush.cpp

	"File Formats/BLP.ixx"
	"File Formats/CASC.ixx"
	"File Formats/INI.ixx"
	"File Formats/JSON.ixx"
	"File Formats/MDX.ixx"
	"File Formats/MPQ.ixx"
	"File Formats/SLK.cpp"

	Resources/CliffMesh.cpp
	Resources/GPUTexture.cpp
	Resources/GroundTexture.cpp
	Resources/Shader.cpp
	Resources/SkinnedMesh.cpp
	Resources/Texture.ixx
	Resources/PathingTexture.ixx
	Resources/QIconResource.cpp

	Resources/SkinnedMesh/RenderNode.ixx
	Resources/SkinnedMesh/SkeletalModelInstance.ixx

	Menus/DoodadPalette.cpp
	
	
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
	Menus/Selections.cpp
	Menus/ObjectEditor.cpp
	Menus/ModelEditor.cpp

	Models/SingleModel.cpp
	Models/TableModel.cpp
	Models/BaseTreeModel.cpp
	Models/UnitTreeModel.cpp
	Models/DoodadTreeModel.cpp
	Models/ItemTreeModel.cpp
	Models/AbilityTreeModel.cpp
	Models/BuffTreeModel.cpp
	Models/UpgradeTreeModel.cpp
	Models/UnitListModel.cpp
	Models/DoodadListModel.cpp
	Models/DestructableListModel.cpp
	Models/DestructibleTreeModel.cpp
	Models/DestructibleTreeModel.h

	"Custom Widgets/AspectRatioPixmapLabel.cpp"
	"Custom Widgets/ColorButton.cpp"
	"Custom Widgets/GLWidget.cpp"
	"Custom Widgets/QRibbon.cpp"
	"Custom Widgets/MainRibbon.cpp"
	"Custom Widgets/UnitSelector.cpp"
	"Custom Widgets/GenericSelectorList.cpp"
	"Custom Widgets/IconView.cpp"
	"Custom Widgets/FlowLayout.cpp"
	"Custom Widgets/ModelEditorGLWidget.cpp"

	"Utilities/Timer.ixx"
	"Utilities/OpenGLUtilities.cpp"
	"Utilities/Utilities.ixx"
	"Utilities/ModificationTables.ixx"
)