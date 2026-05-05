module;
#include <QRect>
#include <QRectF>

export module Rects;

export class PathingRect;

export class TerrainRect: public QRect {
  public:
	using QRect::QRect;
	TerrainRect() = default;

	explicit TerrainRect(const QRect& r) : QRect(r) {}

	TerrainRect intersected(const QRect& r) const {
		return TerrainRect(QRect::intersected(r));
	}

	TerrainRect adjusted(int dx1, int dy1, int dx2, int dy2) const {
		return TerrainRect(QRect::adjusted(dx1, dy1, dx2, dy2));
	}

	/// Converts the TerrainRect into PathingRect. The conversion is lossless.
	PathingRect to_pathing() const;
};

export class TerrainRectF: public QRectF {
  public:
	using QRectF::QRectF;
	TerrainRectF() = default;

	explicit TerrainRectF(const QRectF& r) : QRectF(r) {}

	TerrainRectF adjusted(double dx1, double dy1, double dx2, double dy2) const {
		return TerrainRectF(QRectF::adjusted(dx1, dy1, dx2, dy2));
	}

	TerrainRectF intersected(const QRectF& r) const {
		return TerrainRectF(QRectF::intersected(r));
	}

	TerrainRect toTerrainRect() const;
};

export class PathingRect: public QRect {
  public:
	using QRect::QRect;
	PathingRect() = default;

	explicit PathingRect(const QRect& r) : QRect(r) {}

	PathingRect intersected(const QRect& r) const {
		return PathingRect(QRect::intersected(r));
	}

	PathingRect adjusted(int dx1, int dy1, int dx2, int dy2) const {
		return PathingRect(QRect::adjusted(dx1, dy1, dx2, dy2));
	}

	PathingRect united(const QRect& r) const {
		return PathingRect(QRect::united(r));
	}

	/// Converts the PathingRect to TerrainRect. Returns a minimum rect in
	/// terrain resolution which completely covers the pathing rect
	TerrainRect to_terrain() const;

	/// Converts the PathingRect to TerrainRectF by dividing coordinates by 4. The conversion is lossless.
	TerrainRectF to_terrain_f() const;
};

inline PathingRect TerrainRect::to_pathing() const {
	return PathingRect(x() * 4, y() * 4, width() * 4, height() * 4);
}

inline TerrainRect PathingRect::to_terrain() const {
	int left = x() / 4;
	int top = y() / 4;
	int right = (x() + width() + 3) / 4;
	int bottom = (y() + height() + 3) / 4;
	return TerrainRect(left, top, right - left + 1, bottom - top + 1);
}

inline TerrainRectF PathingRect::to_terrain_f() const {
	return TerrainRectF(x() / 4.0, y() / 4.0, width() / 4.0, height() / 4.0);
}

inline TerrainRect TerrainRectF::toTerrainRect() const {
	return TerrainRect(QRectF::toRect());
}
