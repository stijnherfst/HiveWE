#pragma once

#include <QRect>

template <typename T>
class QuadTree {
	QuadTree* top_left = nullptr;
	QuadTree* top_right = nullptr;
	QuadTree* bottom_left = nullptr;
	QuadTree* bottom_right = nullptr;
	QRect bounds;

	static constexpr int subdivision_limit = 100;

	std::vector<T*> elements;

	void subdivide() {
		const int center_x = bounds.x() + bounds.width() / 2;
		const int center_y = bounds.y() + bounds.height() / 2;

		const int half_width = bounds.width() / 2;
		const int half_height = bounds.height() / 2;

		top_left = new QuadTree<T>({ bounds.x(), bounds.y(), half_width, half_height });
		top_right = new QuadTree<T>({ center_x + 1, bounds.y(), half_width, half_height });
		bottom_left = new QuadTree<T>({ bounds.x(), center_y + 1, half_width, half_height });
		bottom_right = new QuadTree<T>({ center_x + 1, center_y + 1, half_width, half_height });

		for (auto&& i : elements) {
			if (i->position.y < center_y) {
				if (i->position.x < center_x) {
					top_left->insert(i);
				} else {
					top_right->insert(i);
				}
			} else {
				if (i->position.x < center_x) {
					bottom_left->insert(i);
				} else {
					bottom_right->insert(i);
				}
			}
		}
		elements.clear();
	}

	//void join();

public:
	QuadTree() = default;
	QuadTree(const QRect& bounds) : bounds(bounds) {}

	~QuadTree() {
		delete top_left;
		delete top_right;
		delete bottom_left;
		delete bottom_right;
	}

	void resize(int width, int height) {
		bounds.setWidth(width);
		bounds.setHeight(height);
		// Should probably do something with the children
	}

	void insert(T* point) {
		if (top_left) { // Divided
			const int center_x = bounds.x() + bounds.width() / 2;
			const int center_y = bounds.y() + bounds.height() / 2;

			if (point->position.y < center_y) {
				if (point->position.x < center_x) {
					top_left->insert(point);
				} else {
					top_right->insert(point);
				}
			} else {
				if (point->position.x < center_x) {
					bottom_left->insert(point);
				} else {
					bottom_right->insert(point);
				}
			}
		} else {
			elements.push_back(point);

			if (elements.size() > subdivision_limit && bounds.width() > 2 && bounds.height() > 2) {
				subdivide();
			}
		}
	}

	bool remove(T* point) {
		auto pos = std::find(elements.begin(), elements.end(), point);
		if (pos != elements.end()) {
			elements.erase(pos);
			return true;
		}

		return top_left->remove(point)
			|| top_right->remove(point)
			|| bottom_left->remove(point)
			|| bottom_right->remove(point);
	}

	std::vector<T*> query(const QRect& target_bounds) const {
		std::vector<T*> items;
		query(target_bounds, items);
		return items;
	}

	void query(const QRect& target_bounds, std::vector<T*>& result) const {
		if (!bounds.intersects(target_bounds)) {
			return;
		}

		if (target_bounds.contains(bounds) && !top_left) {
			result.insert(result.end(), elements.begin(), elements.end());
		} else {
			if (top_left) { // Divided
				top_left->query(target_bounds, result);
				top_right->query(target_bounds, result);
				bottom_left->query(target_bounds, result);
				bottom_right->query(target_bounds, result);
			} else {
				for (auto&& i : elements) {
					if (target_bounds.contains(QPoint(i->position.x, i->position.y))) {
						result.push_back(i);
					}
				}
			}
		}
	}

	void clear() {
		if (top_left) { // Divided
			top_left->clear();
			top_right->clear();
			bottom_left->clear();
			bottom_right->clear();
		} else {
			elements.clear();
		}
	}
};