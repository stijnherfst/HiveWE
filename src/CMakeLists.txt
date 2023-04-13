target_sources(HiveWE PRIVATE

	"main.cpp"
	
	"Globals.h"
	"MapGlobal.h"
	"unordered_dense.h"

	"MainWindow/HiveWE.rc"
	"MainWindow/HiveWE.cpp"	
	"MainWindow/GLWidget.cpp"
	"MainWindow/MainRibbon.cpp"

	"Base/BinaryReader.ixx"
	"Base/BinaryWriter.ixx"
	"Base/no_init_allocator.ixx"
	"Base/Doodads.cpp"
	"Base/Hierarchy.ixx"
	"Base/GameCameras.ixx"
	"Base/Imports.ixx"
	"Base/Map.cpp"
	"Base/MapInfo.ixx"
	"Base/PathingMap.ixx"
	"Base/Regions.ixx"
	"Base/Camera.ixx"
	"Base/TerrainUndo.ixx"
	"Base/WindowHandler.ixx"
	"Base/MathOperations.ixx"
	"Base/ResourceManager.ixx"
	
	"Base/RenderManager.cpp"
	"Base/ShadowMap.ixx"
	"Base/Sounds.ixx"
	"Base/Terrain.cpp"
	"Base/Triggers.cpp"
	"Base/TriggerStrings.ixx"
	"Base/Units.cpp"
	"Base/Physics.ixx"

	"Brush/Brush.cpp"
	"Brush/DoodadBrush.cpp"
	"Brush/PathingBrush.cpp"
	"Brush/TerrainBrush.cpp"
	"Brush/UnitBrush.cpp"

	"File Formats/BLP.ixx"
	"File Formats/CASC.ixx"
	"File Formats/INI.ixx"
	"File Formats/JSON.ixx"
	"File Formats/MDX/MDX.ixx"
	"File Formats/MDX/MDLReader.cpp"
	"File Formats/MDX/MDLWriter.cpp"
	"File Formats/MDX/MDXReader.cpp"
	"File Formats/MDX/MDXWriter.cpp"
	"File Formats/MPQ.ixx"
	"File Formats/SLK.ixx"
	

	"Resources/CliffMesh.ixx"
	"Resources/GPUTexture.ixx"
	"Resources/GroundTexture.ixx"
	"Resources/Shader.ixx"
	
	"Resources/EditableMesh.cpp"
	"Resources/Texture.ixx"
	"Resources/PathingTexture.ixx"
	"Resources/QIconResource.ixx"

	"Resources/SkinnedMesh/RenderNode.ixx"
	

	"Menus/DoodadPalette.cpp"
	
	"Menus/MapInfoEditor.cpp"
	"Menus/Minimap.cpp"
	"Menus/Palette.cpp"
	"Menus/PathingPalette.cpp"
	"Menus/SettingsEditor.cpp"
	"Menus/TerrainPalette.cpp"
	"Menus/UnitPalette.cpp"
	"Menus/TilePather.cpp"
	"Menus/TilePicker.cpp"
	"Menus/TileSetter.cpp"

	"Models/SingleModel.cpp"
	"Models/TableModel.cpp"
	"Models/BaseTreeModel.ixx"
	"Models/UnitTreeModel.ixx"
	"Models/DoodadTreeModel.ixx"
	"Models/ItemTreeModel.ixx"
	"Models/AbilityTreeModel.ixx"
	"Models/BuffTreeModel.ixx"
	"Models/UpgradeTreeModel.ixx"
	"Models/UnitListModel.ixx"
	"Models/DoodadListModel.cpp"
	
	"Models/DestructibleTreeModel.ixx"

	"Custom Widgets/AspectRatioPixmapLabel.h"
	"Custom Widgets/AspectRatioPixmapLabel.ixx"
	"Custom Widgets/ColorButton.ixx"
	"Custom Widgets/ColorButton.h"
	"Custom Widgets/QRibbon.ixx"
	"Custom Widgets/UnitSelector.ixx"
	"Custom Widgets/FlowLayout.ixx"
	

	"ModelEditor/ModelEditorCamera.cpp"
	"ModelEditor/ModelEditor.cpp"
	"ModelEditor/ModelEditorGLWidget.cpp"

	"ObjectEditor/IconView.cpp"
	"ObjectEditor/ObjectEditor.cpp"

	"QtImgui/ImGuiRenderer.cpp"
	"QtImgui/QtImGui.cpp"

	"Utilities/Timer.ixx"
	"Utilities/Utilities.ixx"
	"Utilities/OpenGLUtilities.ixx"
	
 "Utilities/ModificationTables.ixx" "Models/DestructableListModel.ixx" "Resources/SkinnedMesh/SkeletalModelInstance.ixx" "Resources/SkinnedMesh.ixx" "test.ixx")