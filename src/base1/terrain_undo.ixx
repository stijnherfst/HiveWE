module;

#include <vector>
#include <memory>

export module TerrainUndo;

export class TerrainUndoAction {
  public:
	virtual void undo() = 0;
	virtual void redo() = 0;

	virtual ~TerrainUndoAction() {
	}
};

export class TerrainUndo {
	std::vector<std::vector<std::unique_ptr<TerrainUndoAction>>> undo_actions;
	std::vector<std::vector<std::unique_ptr<TerrainUndoAction>>> redo_actions;

  public:
	void undo() {
		if (undo_actions.empty()) {
			return;
		}

		auto& actions = undo_actions.back();
		for (const auto& i : actions) {
			i->undo();
		}

		redo_actions.push_back(std::move(actions));
		undo_actions.pop_back();
	}

	void redo() {
		if (redo_actions.empty()) {
			return;
		}

		auto& actions = redo_actions.back();
		for (const auto& i : actions) {
			i->redo();
		}

		undo_actions.push_back(std::move(actions));
		redo_actions.pop_back();
	}

	void new_undo_group() {
		undo_actions.push_back({});
	}

	void add_undo_action(std::unique_ptr<TerrainUndoAction> action) {
		if (undo_actions.empty()) {
			return;
		}

		undo_actions.back().push_back(std::move(action));
		redo_actions.clear();
	};
};



