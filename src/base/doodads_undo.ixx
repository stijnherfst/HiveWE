module;

#include <QRectF>

export module DoodadsUndo;

import std;
import Doodad;
import WorldUndoManager;

// Undo/redo structures
export class DoodadAddAction final : public WorldCommand {
public:
	std::vector<Doodad> doodads;

	void undo(WorldEditContext& ctx) override {
		ctx.doodads.doodads.resize(ctx.doodads.doodads.size() - doodads.size());
		ctx.doodads.update_doodad_pathing(doodads, ctx.pathing_map);
	}

	void redo(WorldEditContext& ctx) override {
		ctx.doodads.doodads.insert(ctx.doodads.doodads.end(), doodads.begin(), doodads.end());
		ctx.doodads.update_doodad_pathing(doodads, ctx.pathing_map);
	}
};

export class DoodadDeleteAction final : public WorldCommand {
public:
	std::vector<Doodad> doodads;

	void undo(WorldEditContext& ctx) override {
		if (ctx.brush) {
			ctx.brush->clear_selection();
		}

		ctx.doodads.doodads.insert(ctx.doodads.doodads.end(), doodads.begin(), doodads.end());
		ctx.doodads.update_doodad_pathing(doodads, ctx.pathing_map);
	}

	void redo(WorldEditContext& ctx) override {
		if (ctx.brush) {
			ctx.brush->clear_selection();
		}

		ctx.doodads.doodads.resize(ctx.doodads.doodads.size() - doodads.size());
		ctx.doodads.update_doodad_pathing(doodads, ctx.pathing_map);
	}
};

export class DoodadStateAction final : public WorldCommand {
public:
	std::vector<Doodad> old_doodads;
	std::vector<Doodad> new_doodads;

	void undo(WorldEditContext& ctx) override {
		QRect pathing_area;
		for (const auto& i : old_doodads) {
			for (auto& j : ctx.doodads.doodads) {
				if (i.creation_number == j.creation_number) {
					pathing_area |= i.get_pathing_bounding_box();
					pathing_area |= j.get_pathing_bounding_box();
					j = i;
				}
			}
		}
		ctx.doodads.update_doodad_pathing(pathing_area, ctx.pathing_map);
	}

	void redo(WorldEditContext& ctx) override {
		QRect pathing_area;
		for (const auto& i : new_doodads) {
			for (auto& j : ctx.doodads.doodads) {
				if (i.creation_number == j.creation_number) {
					pathing_area |= i.get_pathing_bounding_box();
					pathing_area |= j.get_pathing_bounding_box();
					j = i;
				}
			}
		}
		ctx.doodads.update_doodad_pathing(pathing_area, ctx.pathing_map);
	}
};
