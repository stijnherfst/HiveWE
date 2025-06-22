module;

#include <QRect>

export module PathingUndo;

import std;
import WorldUndoManager;
import PathingMap;

export class PathingMapAction : public WorldCommand {
public:
	QRect area;
	std::vector<uint8_t> old_pathing;
	std::vector<uint8_t> new_pathing;

	void undo(WorldEditContext& ctx) override {
		for (int j = area.top(); j <= area.bottom(); j++) {
			for (int i = area.left(); i <= area.right(); i++) {
				ctx.pathing_map.pathing_cells_static[j * ctx.pathing_map.width + i] = old_pathing[(j - area.top()) * area.width() + i - area.left()];
			}
		}
		ctx.pathing_map.upload_static_pathing();
	}

	void redo(WorldEditContext& ctx) override {
		for (int j = area.top(); j <= area.bottom(); j++) {
			for (int i = area.left(); i <= area.right(); i++) {
				ctx.pathing_map.pathing_cells_static[j * ctx.pathing_map.width + i] = new_pathing[(j - area.top()) * area.width() + i - area.left()];
			}
		}
		ctx.pathing_map.upload_static_pathing();
	}
};