#pragma once

#include <vector>
#include <memory>

class TerrainUndoAction {
public:
	virtual void undo() = 0;
	virtual void redo() = 0;

	virtual ~TerrainUndoAction() {}
};

class TerrainUndo {
	std::vector<std::vector<std::unique_ptr<TerrainUndoAction>>> undo_actions;
	std::vector<std::vector<std::unique_ptr<TerrainUndoAction>>> redo_actions;

public:
	void undo();
	void redo();

	void new_undo_group();
	void add_undo_action(std::unique_ptr<TerrainUndoAction> action);
};