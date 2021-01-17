target_sources(HiveWE PRIVATE
	HiveWE.rc
	HiveWE.cpp
	main.cpp
	test.cpp
	Utilities.cpp

	Base/Doodads.cpp
	Base/GameCameras.cpp
	Base/Hierarchy.cpp
	Base/Imports.cpp
	Base/Map.cpp
	Base/MapInfo.cpp
	Base/PathingMap.cpp
	Base/Regions.cpp
	Base/Camera.cpp
	Base/InputHandler.cpp
	Base/TerrainUndo.cpp
	Base/WindowHandler.h
	
	Base/RenderManager.cpp
	Base/ShadowMap.cpp
	Base/Sounds.cpp
	Base/Terrain.cpp
	Base/Triggers.cpp
	Base/TriggerStrings.cpp
	Base/Units.cpp
	Base/Physics.cpp

	Brush/Brush.cpp
	Brush/DoodadBrush.cpp
	Brush/PathingBrush.cpp
	Brush/TerrainBrush.cpp
	Brush/UnitBrush.cpp

	"File Formats/BLP.cpp"
	"File Formats/CASC.cpp"
	"File Formats/INI.cpp"
	"File Formats/JSON.cpp"
	"File Formats/MDX.cpp"
	"File Formats/MPQ.cpp"
	"File Formats/SLK.cpp"

	Resources/CliffMesh.cpp
	Resources/GPUTexture.cpp
	Resources/GroundTexture.cpp
	Resources/Shader.cpp
	Resources/SkinnedMesh.cpp
	Resources/StaticMesh.cpp
	Resources/Texture.cpp
	Resources/PathingTexture.cpp
	Resources/QIconResource.cpp

	Resources/SkinnedMesh/RenderNode.cpp
	Resources/SkinnedMesh/SkeletalModelInstance.cpp

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
	Menus/TriggerEditor.cpp
	Menus/Selections.cpp
	Menus/ObjectEditor.cpp

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
	"Custom Widgets/JassEditor.cpp"
	"Custom Widgets/QRibbon.cpp"
	"Custom Widgets/MainRibbon.cpp"
	"Custom Widgets/SearchWindow.cpp"
	"Custom Widgets/VariableEditor.cpp"
	"Custom Widgets/TriggerExplorer.cpp"
	"Custom Widgets/UnitSelector.cpp"
	"Custom Widgets/GenericSelectorList.cpp"
	"Custom Widgets/IconView.cpp"
	"Custom Widgets/JassTokenizer.cpp"
	"Custom Widgets/FlowLayout.cpp"
)