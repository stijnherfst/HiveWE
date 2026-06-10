#pragma once

#include <QObject>

#include <optional>
#include <unordered_set>
#include <vector>
#include <memory>
#include "brush.h"
#include <glm/glm.hpp>

import Regions;
import RegionsUndo;

class RegionBrush : public Brush {
	Q_OBJECT

  public:
	std::unordered_set<Region*> selections;

	RegionBrush();

	void key_press_event(QKeyEvent* event) override;
	void mouse_press_event(QMouseEvent* event, double frame_delta) override;
	void mouse_move_event(QMouseEvent* event, double frame_delta) override;
	void mouse_release_event(QMouseEvent* event) override;

	void delete_selection() override;
	void clear_selection() override;

	void apply(double frame_delta) override;

	void start_action();
	void end_action();

  private:
	/// Which edges of a region a resize drag moves. Both a horizontal and a vertical edge are set when dragging a corner
	struct ResizeEdges {
		bool left = false;
		bool right = false;
		bool bottom = false;
		bool top = false;
	};

	/// Regions snap to the cell grid (a quarter of a tile)
	static glm::vec2 snap(const glm::vec2 position) {
		return glm::round(position * 4.f) / 4.f;
	}

	float border_size() const;
	float corner_size() const;

	/// Returns the edges to resize when grabbing the border/corner of the region at this position
	std::optional<ResizeEdges> edges_under(const Region& region, glm::vec2 position) const;

	/// All regions under the mouse with the topmost (drawn last) first
	std::vector<Region*> regions_under() const;

	/// Whether the resize in progress is dragging out a newly created region
	bool creating = false;

	bool dragging = false;
	glm::vec2 drag_start;

	bool resizing = false;
	Region* resize_region = nullptr;
	ResizeEdges resize_edges;

	/// Whether the mouse moved to another cell since the press, distinguishing drags from plain clicks
	bool dragged = false;
	glm::vec2 press_position;
	/// Whether releasing without dragging should cycle the selection through the regions under the mouse
	bool select_on_release = false;

	std::unique_ptr<RegionStateAction> state_undo;

  signals:
	void regions_changed();
};
