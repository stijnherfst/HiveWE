target_sources(HiveWE PUBLIC FILE_SET yeet TYPE CXX_MODULES FILES
	"types.ixx"
	"map_global.ixx"
	"globals.ixx"

	"base/map.ixx"
	"base/binary_reader.ixx"
	"base/doodads.ixx"
	"base/binary_writer.ixx"
	"base/hierarchy.ixx"
	"base/render_manager.ixx"
	"base/game_cameras.ixx"
	"base/imports.ixx"
	"base/map_info.ixx"
	"base/pathing_map.ixx"
	"base/regions.ixx"
	"base/camera.ixx"
	"base/world_undo_manager.ixx"
	"base/window_handler.ixx"
	"base/resource_manager.ixx"
	"base/shadow_map.ixx"
	"base/sounds.ixx"
	"base/trigger_strings.ixx"
	"base/physics.ixx"
	"base/terrain_undo.ixx"
	"base/units_undo.ixx"
	"base/pathing_undo.ixx"
	"base/doodads_undo.ixx"
	"base/units.ixx"
	"base/terrain.ixx"

	"file_formats/blp.ixx"
	"file_formats/casc.ixx"
	"file_formats/ini.ixx"
	"file_formats/json.ixx"
	"file_formats/mdx/mdx.ixx"

	"file_formats/mpq.ixx"
	"file_formats/slk.ixx"

	"resources/cliff_mesh.ixx"
	"resources/gpu_texture.ixx"
	"resources/ground_texture.ixx"
	"resources/shader.ixx"

	"resources/texture.ixx"
	"resources/pathing_texture.ixx"
	"resources/qicon_resource.ixx"
	"resources/editable_mesh.ixx"
	"resources/skinned_mesh/render_node.ixx"
	"resources/skinned_mesh/skeletal_model_instance.ixx" 
	"resources/skinned_mesh.ixx" 

	"models/base_tree_model.ixx"
	"models/unit_tree_model.ixx"
	"models/doodad_tree_model.ixx"
	"models/item_tree_model.ixx"
	"models/ability_tree_model.ixx"
	"models/buff_tree_model.ixx"
	"models/upgrade_tree_model.ixx"
	"models/unit_list_model.ixx"
	"models/table_model.ixx"
	"models/destructible_tree_model.ixx"

	"object_editor/slk_conversions.ixx"

	"custom_widgets/aspect_ratio_pixmap_label.ixx"
	"custom_widgets/color_button.ixx"

	"custom_widgets/qribbon.ixx"
	"custom_widgets/unit_selector.ixx"
	"custom_widgets/flow_layout.ixx"

	"utilities/timer.ixx"
	"utilities/utilities.ixx"
	"utilities/opengl_utilities.ixx"
	"utilities/modification_tables.ixx" 
	 
	"utilities/no_init_allocator.ixx"
	"utilities/math_operations.ixx"
	
	"test.ixx"
	"object_editor/ability_list_editor.ixx"
)

target_sources(HiveWE PRIVATE
	"main.cpp"

	"main_window/hivewe.rc"
	"main_window/hivewe.cpp"	
	"main_window/glwidget.cpp"
	"main_window/main_ribbon.cpp"

	"brush/brush.cpp"
	"brush/doodad_brush.cpp"
	"brush/pathing_brush.cpp"
	"brush/terrain_brush.cpp"
	"brush/unit_brush.cpp"
	
	"file_formats/mdx/mdl_reader.cpp"
	"file_formats/mdx/mdl_writer.cpp"
	"file_formats/mdx/mdx_reader.cpp"
	"file_formats/mdx/mdx_writer.cpp"
	"file_formats/mdx/optimizer.cpp"
	"file_formats/mdx/utilities.cpp"
	"file_formats/mdx/validator.cpp"


	"menus/doodad_palette.cpp"
	"menus/map_info_editor.cpp"
	"menus/minimap.cpp"
	"menus/palette.cpp"
	"menus/pathing_palette.cpp"
	"menus/settings_editor.cpp"
	"menus/terrain_palette.cpp"
	"menus/unit_palette.cpp"
	"menus/tile_pather.cpp"
	"menus/tile_picker.cpp"
	"menus/tile_setter.cpp"

	"base/triggers.cpp"

	"models/single_model.cpp"
	
	"custom_widgets/aspect_ratio_pixmap_label.h"
	"custom_widgets/color_button.h"
	
	"model_editor/model_editor_camera.cpp"
	"model_editor/model_editor.cpp"
	"model_editor/model_editor_glwidget.cpp"
	"models/doodad_list_model.cpp"
	"models/destructible_list_model.cpp"

	"object_editor/icon_view.cpp"
	"object_editor/object_editor.cpp"

	"qt_imgui/imgui_renderer.cpp"
	"qt_imgui/qt_imgui.cpp"

	
 )