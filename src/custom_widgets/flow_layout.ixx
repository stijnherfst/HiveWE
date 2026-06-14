module;

#include <QLayout>
#include <QLayoutItem>
#include <QStyle>
#include <QWidget>

export module FlowLayout;

// FlowLayout is a gridlike layout that will adjust the position of the items according to screen width
export class FlowLayout: public QLayout {
  public:
	explicit FlowLayout(QWidget* parent, int margin = -1, int h_spacing = -1, int v_spacing = -1) :
		QLayout(parent),
		h_space(h_spacing),
		v_space(v_spacing) {
		setContentsMargins(margin, margin, margin, margin);
	}

	explicit FlowLayout(int margin = -1, int h_spacing = -1, int v_spacing = -1) : h_space(h_spacing), v_space(v_spacing) {
		setContentsMargins(margin, margin, margin, margin);
	}

	~FlowLayout() override {
		while (QLayoutItem* item = FlowLayout::takeAt(0)) {
			delete item;
		}
	}

	void addItem(QLayoutItem* item) override {
		item_list.append(item);

		invalidate();
		if (QWidget* pw = parentWidget()) {
			pw->updateGeometry();
		}
	}

	void insert_widget(const int index, QWidget* widget) {
		addWidget(widget);

		const int current = indexOf(widget);
		if (current < 0) {
			return;
		}

		const int dest = std::clamp(index, 0, count() - 1);
		item_list.move(current, dest);
		invalidate();
	}

	void move_widget(const int index, const QWidget* widget) {
		const int old_index = indexOf(widget);
		if (old_index < 0) {
			return;
		}

		const int dest = std::clamp(index, 0, std::max(0, count() - 1));
		item_list.move(old_index, dest);
		invalidate();
	}

	[[nodiscard]]
	int horizontal_spacing() const {
		if (h_space >= 0) {
			return h_space;
		}
		return smart_spacing(QStyle::PM_LayoutHorizontalSpacing);
	}

	[[nodiscard]]
	int vertical_spacing() const {
		if (v_space >= 0) {
			return v_space;
		}
		return smart_spacing(QStyle::PM_LayoutVerticalSpacing);
	}

	[[nodiscard]]
	int count() const override {
		return item_list.size();
	}

	[[nodiscard]]
	QLayoutItem* itemAt(const int index) const override {
		return item_list.value(index);
	}

	QLayoutItem* takeAt(const int index) override {
		if (index >= 0 && index < item_list.size()) {
			return item_list.takeAt(index);
		} else {
			return nullptr;
		}
	}

	[[nodiscard]]
	Qt::Orientations expandingDirections() const override {
		return Qt::Orientations();
	}

	[[nodiscard]]
	bool hasHeightForWidth() const override {
		return true;
	}

	// int heightForWidth(const int width) const {
	//	const int height = do_layout(QRect(0, 0, width, 0), true);
	//	return height;
	// }
	[[nodiscard]]
	int heightForWidth(const int width) const override {
		const int height = do_layout(QRect(0, 0, width, 0), true);
		return height;
	}

	void setGeometry(const QRect& rect) override {
		QLayout::setGeometry(rect);
		do_layout(rect, false);
	}

	[[nodiscard]]
	QSize sizeHint() const override {
		return minimumSize();
	}

	// QSize minimumSize() const {
	//	QSize size;
	//	for (auto&& item : item_list) {
	//		size = size.expandedTo(item->minimumSize());
	//	}
	//
	//	size += QSize(2 * margin(), 2 * margin());
	//	return size;
	// }

	[[nodiscard]]
	QSize minimumSize() const override {
		QSize size;

		for (const auto* item : item_list) {
			size = size.expandedTo(item->minimumSize());
		}

		const QMargins m = contentsMargins();
		size += QSize(m.left() + m.right(), m.top() + m.bottom());
		return size;
	}

	[[nodiscard]]
	QList<QLayoutItem*> items() const {
		return item_list;
	}

	void clear() {
		for (auto&& i : item_list) {
			if (QWidget* w = i->widget()) {
				w->deleteLater();
			}
			delete i;
		}
		item_list.clear();
		invalidate();
	}

	// int do_layout(const QRect &rect, const bool test_only) const {
	//	int left, top, right, bottom;
	//	getContentsMargins(&left, &top, &right, &bottom);
	//	QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
	//	int x = effectiveRect.x();
	//	int y = effectiveRect.y();
	//	int lineHeight = 0;
	//
	//	for (auto&& item : item_list) {
	//		QWidget *wid = item->widget();
	//		int spaceX = horizontal_spacing();
	//		if (spaceX == -1)
	//			spaceX = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
	//		int spaceY = vertical_spacing();
	//		if (spaceY == -1)
	//			spaceY = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
	//		int nextX = x + item->sizeHint().width() + spaceX;
	//		if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
	//			x = effectiveRect.x();
	//			y = y + lineHeight + spaceY;
	//			nextX = x + item->sizeHint().width() + spaceX;
	//			lineHeight = 0;
	//		}
	//
	//		if (!test_only)
	//			item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
	//
	//		x = nextX;
	//		lineHeight = qMax(lineHeight, item->sizeHint().height());
	//	}
	//	return y + lineHeight - rect.y() + bottom;
	// }

	int do_layout(const QRect& rect, const bool test_only) const {
		int left, top, right, bottom;
		getContentsMargins(&left, &top, &right, &bottom);
		QRect effective_rect = rect.adjusted(+left, +top, -right, -bottom);
		int x = effective_rect.x();
		int y = effective_rect.y();
		int line_height = 0;

		for (QLayoutItem* item : item_list) {
			const QWidget* wid = item->widget();
			if (!wid) {
				continue;
			}

			const QSize hint = item->sizeHint();
			const int hint_width = hint.width();
			const int hint_height = hint.height();

			int space_x = horizontal_spacing();
			if (space_x == -1) {
				space_x = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
			}

			int space_y = vertical_spacing();
			if (space_y == -1) {
				space_y = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
			}

			int next_x = x + hint_width + space_x;
			if (next_x - space_x > effective_rect.right() && line_height > 0) {
				x = effective_rect.x();
				y = y + line_height + space_y;
				next_x = x + hint_width + space_x;
				line_height = 0;
			}

			if (!test_only) {
				item->setGeometry(QRect(QPoint(x, y), hint));
			}

			x = next_x;
			line_height = qMax(line_height, hint_height);
		}

		const int height = y + line_height - rect.y() + bottom;
		return height;
	}

	[[nodiscard]]
	int smart_spacing(const QStyle::PixelMetric pm) const {
		QObject* parent = this->parent();
		if (!parent) {
			return -1;
		}

		if (parent->isWidgetType()) {
			const QWidget* pw = qobject_cast<QWidget*>(parent);
			return pw->style()->pixelMetric(pm, nullptr, pw);
		}

		if (const auto* layout = qobject_cast<QLayout*>(parent)) {
			return layout->spacing();
		}

		return -1;
	}

	QList<QLayoutItem*> item_list;
	int h_space;
	int v_space;
};
