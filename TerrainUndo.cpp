#include "stdafx.h"

void TerrainUndo::undo() {
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

void TerrainUndo::redo() {
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

void TerrainUndo::new_undo_group() {
	undo_actions.push_back({});
}

void TerrainUndo::add_undo_action(std::unique_ptr<TerrainUndoAction> action) {
	if (undo_actions.empty()) {
		return;
	}

	undo_actions.back().push_back(std::move(action));
	redo_actions.clear();
}