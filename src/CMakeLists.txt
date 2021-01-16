target_sources(HiveWE PRIVATE
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
)