export module WorldUndoManager;

import std;
import PathingMap;
import Units;
import Doodads;
import Terrain;
import "brush/brush.h";

/// So we don't have to pass a global variable around
export struct WorldEditContext {
	Terrain& terrain;
	Units& units;
	Doodads& doodads;
	Brush* brush;
	PathingMap& pathing_map;
};

/// Describes a change in the world. E.g., changing the terrain, adding a unit in the OE, placing some doodads.
export class WorldCommand {
  public:
	virtual void undo(WorldEditContext& ctx) = 0;
	virtual void redo(WorldEditContext& ctx) = 0;

	virtual ~WorldCommand() = default;
};

export class WorldUndoManager {
	std::vector<std::vector<std::unique_ptr<WorldCommand>>> undo_actions;
	std::vector<std::vector<std::unique_ptr<WorldCommand>>> redo_actions;

  public:
	void undo(WorldEditContext& ctx) {
		if (undo_actions.empty()) {
			return;
		}

		auto& actions = undo_actions.back();
		for (const auto& i : actions) {
			i->undo(ctx);
		}

		redo_actions.push_back(std::move(actions));
		undo_actions.pop_back();
	}

	void redo(WorldEditContext& ctx) {
		if (redo_actions.empty()) {
			return;
		}

		auto& actions = redo_actions.back();
		for (const auto& i : actions) {
			i->redo(ctx);
		}

		undo_actions.push_back(std::move(actions));
		redo_actions.pop_back();
	}

	void new_undo_group() {
		undo_actions.push_back({});
	}

	void add_undo_action(std::unique_ptr<WorldCommand> action) {
		if (undo_actions.empty()) {
			return;
		}

		undo_actions.back().push_back(std::move(action));
		redo_actions.clear();
	};
};



