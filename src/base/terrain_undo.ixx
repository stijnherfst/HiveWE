module;

#include <QRect>

export module TerrainUndo;

import Terrain;
import Units;
import WorldUndoManager;

export enum class TerrainUndoType {
	texture,
	height,
	cliff,
	water
};

export class TerrainGenericAction final : public WorldCommand {
public:
	QRect area;
	std::vector<Corner> old_corners;
	std::vector<Corner> new_corners;
	TerrainUndoType undo_type;

	void undo(WorldEditContext& ctx) override {
		for (int j = area.top(); j <= area.bottom(); j++) {
			for (int i = area.left(); i <= area.right(); i++) {
				ctx.terrain.corners[i][j] = old_corners[(j - area.top()) * area.width() + i - area.left()];
			}
		}

		if (undo_type == TerrainUndoType::height) {
			ctx.terrain.update_ground_heights(area);
		}

		if (undo_type == TerrainUndoType::texture) {
			ctx.terrain.update_ground_textures(area);
		}

		if (undo_type == TerrainUndoType::cliff) {
			ctx.terrain.update_ground_heights(area);
			ctx.terrain.update_cliff_meshes(area);
			ctx.terrain.update_ground_textures(area);
			ctx.terrain.update_water(area);
		}

		ctx.terrain.update_minimap();
		ctx.units.update_area(area, ctx.terrain);
	}

	void redo(WorldEditContext& ctx) override {
		for (int j = area.top(); j <= area.bottom(); j++) {
			for (int i = area.left(); i <= area.right(); i++) {
				ctx.terrain.corners[i][j] = new_corners[(j - area.top()) * area.width() + i - area.left()];
			}
		}

		if (undo_type == TerrainUndoType::height) {
			ctx.terrain.update_ground_heights(area);
		}

		if (undo_type == TerrainUndoType::texture) {
			ctx.terrain.update_ground_textures(area);
		}

		if (undo_type == TerrainUndoType::cliff) {
			ctx.terrain.update_ground_heights(area);
			ctx.terrain.update_cliff_meshes(area);
			ctx.terrain.update_ground_textures(area);
			ctx.terrain.update_water(area);
		}

		ctx.terrain.update_minimap();
		ctx.units.update_area(area, ctx.terrain);
	}
};