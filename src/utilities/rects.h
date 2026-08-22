#pragma once

namespace rect_detail {
template<typename T>
constexpr T min_value(T a, T b) noexcept {
    return a < b ? a : b;
}

template<typename T>
constexpr T max_value(T a, T b) noexcept {
    return a > b ? a : b;
}

constexpr int round_nearest(double value) noexcept {
    return static_cast<int>(
        value >= 0.0 ? value + 0.5 : value - 0.5
    );
}
}

class PathingRect;
class TerrainRectF;

class TerrainRect {
    int x1_ = 0;
    int y1_ = 0;
    int x2_ = -1;
    int y2_ = -1;

    static TerrainRect from_edges(
        int left,
        int top,
        int right,
        int bottom
    ) noexcept {
        TerrainRect result;
        result.x1_ = left;
        result.y1_ = top;
        result.x2_ = right;
        result.y2_ = bottom;
        return result;
    }

  public:
    constexpr TerrainRect() noexcept = default;

    constexpr TerrainRect(
        int x,
        int y,
        int width,
        int height
    ) noexcept
        : x1_(x),
          y1_(y),
          x2_(x + width - 1),
          y2_(y + height - 1) {}

    [[nodiscard]] constexpr int x() const noexcept {
        return x1_;
    }

    [[nodiscard]] constexpr int y() const noexcept {
        return y1_;
    }

    [[nodiscard]] constexpr int left() const noexcept {
        return x1_;
    }

    [[nodiscard]] constexpr int top() const noexcept {
        return y1_;
    }

    [[nodiscard]] constexpr int right() const noexcept {
        return x2_;
    }

    [[nodiscard]] constexpr int bottom() const noexcept {
        return y2_;
    }

    [[nodiscard]] constexpr int width() const noexcept {
        return x2_ - x1_ + 1;
    }

    [[nodiscard]] constexpr int height() const noexcept {
        return y2_ - y1_ + 1;
    }

    [[nodiscard]] constexpr bool isNull() const noexcept {
        return width() == 0 && height() == 0;
    }

    [[nodiscard]] constexpr bool isEmpty() const noexcept {
        return width() <= 0 || height() <= 0;
    }

    [[nodiscard]] constexpr bool isValid() const noexcept {
        return width() > 0 && height() > 0;
    }

    constexpr void setX(int value) noexcept {
        x1_ = value;
    }

    constexpr void setY(int value) noexcept {
        y1_ = value;
    }

    constexpr void setLeft(int value) noexcept {
        x1_ = value;
    }

    constexpr void setTop(int value) noexcept {
        y1_ = value;
    }

    constexpr void setRight(int value) noexcept {
        x2_ = value;
    }

    constexpr void setBottom(int value) noexcept {
        y2_ = value;
    }

    constexpr void setWidth(int value) noexcept {
        x2_ = x1_ + value - 1;
    }

    constexpr void setHeight(int value) noexcept {
        y2_ = y1_ + value - 1;
    }

    [[nodiscard]] constexpr bool contains(
        int px,
        int py
    ) const noexcept {
        return !isEmpty()
            && px >= x1_
            && px <= x2_
            && py >= y1_
            && py <= y2_;
    }

    [[nodiscard]] constexpr bool intersects(
        const TerrainRect& other
    ) const noexcept {
        return !isEmpty()
            && !other.isEmpty()
            && x1_ <= other.x2_
            && other.x1_ <= x2_
            && y1_ <= other.y2_
            && other.y1_ <= y2_;
    }

    [[nodiscard]] constexpr TerrainRect intersected(
        const TerrainRect& other
    ) const noexcept {
        if (!intersects(other)) {
            return {};
        }

        return from_edges(
            rect_detail::max_value(x1_, other.x1_),
            rect_detail::max_value(y1_, other.y1_),
            rect_detail::min_value(x2_, other.x2_),
            rect_detail::min_value(y2_, other.y2_)
        );
    }

    [[nodiscard]] constexpr TerrainRect adjusted(
        int dx1,
        int dy1,
        int dx2,
        int dy2
    ) const noexcept {
        return from_edges(
            x1_ + dx1,
            y1_ + dy1,
            x2_ + dx2,
            y2_ + dy2
        );
    }

    [[nodiscard]] constexpr TerrainRect united(
        const TerrainRect& other
    ) const noexcept {
        if (isEmpty()) {
            return other;
        }

        if (other.isEmpty()) {
            return *this;
        }

        return from_edges(
            rect_detail::min_value(x1_, other.x1_),
            rect_detail::min_value(y1_, other.y1_),
            rect_detail::max_value(x2_, other.x2_),
            rect_detail::max_value(y2_, other.y2_)
        );
    }

    constexpr TerrainRect& operator|=(
        const TerrainRect& other
    ) noexcept {
        *this = united(other);
        return *this;
    }

    [[nodiscard]] constexpr TerrainRect operator|(
        const TerrainRect& other
    ) const noexcept {
        return united(other);
    }

    constexpr void moveTo(int new_x, int new_y) noexcept {
        const int current_width = width();
        const int current_height = height();

        x1_ = new_x;
        y1_ = new_y;
        x2_ = new_x + current_width - 1;
        y2_ = new_y + current_height - 1;
    }

    constexpr void moveLeft(int value) noexcept {
        moveTo(value, y1_);
    }

    constexpr void moveTop(int value) noexcept {
        moveTo(x1_, value);
    }

    [[nodiscard]] PathingRect to_pathing() const noexcept;

    constexpr bool operator==(
        const TerrainRect&
    ) const noexcept = default;
};

class TerrainRectF {
    double x_ = 0.0;
    double y_ = 0.0;
    double width_ = 0.0;
    double height_ = 0.0;

  public:
    constexpr TerrainRectF() noexcept = default;

    constexpr TerrainRectF(
        double x,
        double y,
        double width,
        double height
    ) noexcept
        : x_(x),
          y_(y),
          width_(width),
          height_(height) {}

    constexpr TerrainRectF(
        const TerrainRect& rect
    ) noexcept
        : x_(rect.x()),
          y_(rect.y()),
          width_(rect.width()),
          height_(rect.height()) {}

    [[nodiscard]] constexpr double x() const noexcept {
        return x_;
    }

    [[nodiscard]] constexpr double y() const noexcept {
        return y_;
    }

    [[nodiscard]] constexpr double left() const noexcept {
        return x_;
    }

    [[nodiscard]] constexpr double top() const noexcept {
        return y_;
    }

    [[nodiscard]] constexpr double right() const noexcept {
        return x_ + width_;
    }

    [[nodiscard]] constexpr double bottom() const noexcept {
        return y_ + height_;
    }

    [[nodiscard]] constexpr double width() const noexcept {
        return width_;
    }

    [[nodiscard]] constexpr double height() const noexcept {
        return height_;
    }

    [[nodiscard]] constexpr bool isNull() const noexcept {
        return width_ == 0.0 && height_ == 0.0;
    }

    [[nodiscard]] constexpr bool isEmpty() const noexcept {
        return width_ <= 0.0 || height_ <= 0.0;
    }

    [[nodiscard]] constexpr bool isValid() const noexcept {
        return width_ > 0.0 && height_ > 0.0;
    }

    [[nodiscard]] constexpr bool contains(
        double px,
        double py
    ) const noexcept {
        return !isEmpty()
            && px >= left()
            && px <= right()
            && py >= top()
            && py <= bottom();
    }

    [[nodiscard]] constexpr bool intersects(
        const TerrainRectF& other
    ) const noexcept {
        return !isEmpty()
            && !other.isEmpty()
            && left() < other.right()
            && other.left() < right()
            && top() < other.bottom()
            && other.top() < bottom();
    }

    [[nodiscard]] constexpr TerrainRectF adjusted(
        double dx1,
        double dy1,
        double dx2,
        double dy2
    ) const noexcept {
        return TerrainRectF(
            x_ + dx1,
            y_ + dy1,
            width_ + dx2 - dx1,
            height_ + dy2 - dy1
        );
    }

    [[nodiscard]] constexpr TerrainRectF intersected(
        const TerrainRectF& other
    ) const noexcept {
        const double new_left =
            rect_detail::max_value(left(), other.left());
        const double new_top =
            rect_detail::max_value(top(), other.top());
        const double new_right =
            rect_detail::min_value(right(), other.right());
        const double new_bottom =
            rect_detail::min_value(bottom(), other.bottom());

        if (new_right <= new_left || new_bottom <= new_top) {
            return {};
        }

        return TerrainRectF(
            new_left,
            new_top,
            new_right - new_left,
            new_bottom - new_top
        );
    }

    [[nodiscard]] constexpr TerrainRectF united(
        const TerrainRectF& other
    ) const noexcept {
        if (isEmpty()) {
            return other;
        }

        if (other.isEmpty()) {
            return *this;
        }

        const double new_left =
            rect_detail::min_value(left(), other.left());
        const double new_top =
            rect_detail::min_value(top(), other.top());
        const double new_right =
            rect_detail::max_value(right(), other.right());
        const double new_bottom =
            rect_detail::max_value(bottom(), other.bottom());

        return TerrainRectF(
            new_left,
            new_top,
            new_right - new_left,
            new_bottom - new_top
        );
    }

    constexpr TerrainRectF& operator|=(
        const TerrainRectF& other
    ) noexcept {
        *this = united(other);
        return *this;
    }

    [[nodiscard]] constexpr TerrainRectF operator|(
        const TerrainRectF& other
    ) const noexcept {
        return united(other);
    }

    [[nodiscard]] constexpr TerrainRect to_terrain_rect() const noexcept {
        return TerrainRect(
            rect_detail::round_nearest(x_),
            rect_detail::round_nearest(y_),
            rect_detail::round_nearest(width_),
            rect_detail::round_nearest(height_)
        );
    }

    constexpr bool operator==(
        const TerrainRectF&
    ) const noexcept = default;
};

class PathingRect {
    int x1_ = 0;
    int y1_ = 0;
    int x2_ = -1;
    int y2_ = -1;

    static PathingRect from_edges(
        int left,
        int top,
        int right,
        int bottom
    ) noexcept {
        PathingRect result;
        result.x1_ = left;
        result.y1_ = top;
        result.x2_ = right;
        result.y2_ = bottom;
        return result;
    }

  public:
    constexpr PathingRect() noexcept = default;

    constexpr PathingRect(
        int x,
        int y,
        int width,
        int height
    ) noexcept
        : x1_(x),
          y1_(y),
          x2_(x + width - 1),
          y2_(y + height - 1) {}

    [[nodiscard]] constexpr int x() const noexcept {
        return x1_;
    }

    [[nodiscard]] constexpr int y() const noexcept {
        return y1_;
    }

    [[nodiscard]] constexpr int left() const noexcept {
        return x1_;
    }

    [[nodiscard]] constexpr int top() const noexcept {
        return y1_;
    }

    [[nodiscard]] constexpr int right() const noexcept {
        return x2_;
    }

    [[nodiscard]] constexpr int bottom() const noexcept {
        return y2_;
    }

    [[nodiscard]] constexpr int width() const noexcept {
        return x2_ - x1_ + 1;
    }

    [[nodiscard]] constexpr int height() const noexcept {
        return y2_ - y1_ + 1;
    }

    [[nodiscard]] constexpr bool isNull() const noexcept {
        return width() == 0 && height() == 0;
    }

    [[nodiscard]] constexpr bool isEmpty() const noexcept {
        return width() <= 0 || height() <= 0;
    }

    [[nodiscard]] constexpr bool isValid() const noexcept {
        return width() > 0 && height() > 0;
    }

    constexpr void setX(int value) noexcept {
        x1_ = value;
    }

    constexpr void setY(int value) noexcept {
        y1_ = value;
    }

    constexpr void setLeft(int value) noexcept {
        x1_ = value;
    }

    constexpr void setTop(int value) noexcept {
        y1_ = value;
    }

    constexpr void setRight(int value) noexcept {
        x2_ = value;
    }

    constexpr void setBottom(int value) noexcept {
        y2_ = value;
    }

    constexpr void setWidth(int value) noexcept {
        x2_ = x1_ + value - 1;
    }

    constexpr void setHeight(int value) noexcept {
        y2_ = y1_ + value - 1;
    }

    [[nodiscard]] constexpr bool contains(
        int px,
        int py
    ) const noexcept {
        return !isEmpty()
            && px >= x1_
            && px <= x2_
            && py >= y1_
            && py <= y2_;
    }

    [[nodiscard]] constexpr bool intersects(
        const PathingRect& other
    ) const noexcept {
        return !isEmpty()
            && !other.isEmpty()
            && x1_ <= other.x2_
            && other.x1_ <= x2_
            && y1_ <= other.y2_
            && other.y1_ <= y2_;
    }

    [[nodiscard]] constexpr PathingRect intersected(
        const PathingRect& other
    ) const noexcept {
        if (!intersects(other)) {
            return {};
        }

        return from_edges(
            rect_detail::max_value(x1_, other.x1_),
            rect_detail::max_value(y1_, other.y1_),
            rect_detail::min_value(x2_, other.x2_),
            rect_detail::min_value(y2_, other.y2_)
        );
    }

    [[nodiscard]] constexpr PathingRect adjusted(
        int dx1,
        int dy1,
        int dx2,
        int dy2
    ) const noexcept {
        return from_edges(
            x1_ + dx1,
            y1_ + dy1,
            x2_ + dx2,
            y2_ + dy2
        );
    }

    [[nodiscard]] constexpr PathingRect united(
        const PathingRect& other
    ) const noexcept {
        if (isEmpty()) {
            return other;
        }

        if (other.isEmpty()) {
            return *this;
        }

        return from_edges(
            rect_detail::min_value(x1_, other.x1_),
            rect_detail::min_value(y1_, other.y1_),
            rect_detail::max_value(x2_, other.x2_),
            rect_detail::max_value(y2_, other.y2_)
        );
    }

    constexpr PathingRect& operator|=(
        const PathingRect& other
    ) noexcept {
        *this = united(other);
        return *this;
    }

    [[nodiscard]] constexpr PathingRect operator|(
        const PathingRect& other
    ) const noexcept {
        return united(other);
    }

    [[nodiscard]] constexpr TerrainRect to_terrain() const noexcept {
        const int new_left = x() / 4;
        const int new_top = y() / 4;
        const int new_right = (x() + width() + 3) / 4;
        const int new_bottom = (y() + height() + 3) / 4;

        return TerrainRect(
            new_left,
            new_top,
            new_right - new_left + 1,
            new_bottom - new_top + 1
        );
    }

    [[nodiscard]] constexpr TerrainRectF to_terrain_f() const noexcept {
        return TerrainRectF(
            x() / 4.0,
            y() / 4.0,
            width() / 4.0,
            height() / 4.0
        );
    }

    constexpr bool operator==(
        const PathingRect&
    ) const noexcept = default;
};

inline PathingRect TerrainRect::to_pathing() const noexcept {
    return PathingRect(
        x() * 4,
        y() * 4,
        width() * 4,
        height() * 4
    );
}
