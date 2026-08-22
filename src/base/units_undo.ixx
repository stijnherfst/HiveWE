module;

#ifdef __linux__
#include "std_compat.h"
#endif
#include "brush/brush_render_bridge.h"


export module UnitsUndo;

#ifndef __linux__
import std;
#endif
import Units;
import WorldUndoManager;

// Undo/redo structures
export class UnitAddAction final : public WorldCommand {
public:
	std::vector<Unit> units;

	void undo(WorldEditContext& ctx) override {
		ctx.units.units.resize(ctx.units.units.size() - units.size());
	}

	void redo(WorldEditContext& ctx) override {
		ctx.units.units.insert(ctx.units.units.end(), units.begin(), units.end());
	}
};

export class UnitDeleteAction final : public WorldCommand {
public:
	std::vector<Unit> units;

	void undo(WorldEditContext& ctx) override {
		if (ctx.brush) {
			clear_brush_selection(ctx.brush);
		}

		ctx.units.units.insert(ctx.units.units.end(), units.begin(), units.end());
	}

	void redo(WorldEditContext& ctx) override {
		if (ctx.brush) {
			clear_brush_selection(ctx.brush);
		}

		ctx.units.units.resize(ctx.units.units.size() - units.size());
	}
};

export class UnitStateAction final : public WorldCommand {
public:
	std::vector<Unit> old_units;
	std::vector<Unit> new_units;

	void undo(WorldEditContext& ctx) override {
		for (auto& i : old_units) {
			for (auto& j : ctx.units.units) {
				if (i.creation_number == j.creation_number) {
					j = i;
				}
			}
		}
	}

	void redo(WorldEditContext& ctx) override {
		for (auto& i : new_units) {
			for (auto& j : ctx.units.units) {
				if (i.creation_number == j.creation_number) {
					j = i;
				}
			}
		}
	}
};






