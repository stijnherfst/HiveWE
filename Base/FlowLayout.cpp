#include "FlowLayout.h"

#include <QStyle>
#include <QWidget>

FlowLayout::FlowLayout(QWidget *parent, const int margin, const int h_spacing, const int v_spacing) : QLayout(parent), h_space(h_spacing), v_space(v_spacing) {
	setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::FlowLayout(const int margin, const int hSpacing, const int vSpacing) : h_space(hSpacing), v_space(vSpacing) {
	setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout() {
	QLayoutItem *item;
	while ((item = FlowLayout::takeAt(0)))
		delete item;
}

void FlowLayout::addItem(QLayoutItem *item) {
	item_list.append(item);
}

void FlowLayout::insert_widget(const int index, QWidget* widget) {
	addWidget(widget);
	item_list.move(indexOf(widget), index);
}

void FlowLayout::move_widget(const int index, QWidget* widget) {
	if (index >= 0 && index < count()) {
		item_list.move(indexOf(widget), index);
		update();
	}
}

int FlowLayout::horizontal_spacing() const {
	if (h_space >= 0) {
		return h_space;
	} else {
		return smart_spacing(QStyle::PM_LayoutHorizontalSpacing);
	}
}

int FlowLayout::vertical_spacing() const {
	if (v_space >= 0) {
		return v_space;
	} else {
		return smart_spacing(QStyle::PM_LayoutVerticalSpacing);
	}
}

int FlowLayout::count() const {
	return item_list.size();
}

QLayoutItem *FlowLayout::itemAt(const int index) const {
	return item_list.value(index);
}

QLayoutItem *FlowLayout::takeAt(const int index) {
	if (index >= 0 && index < item_list.size())
		return item_list.takeAt(index);
	else
		return nullptr;
}

Qt::Orientations FlowLayout::expandingDirections() const {
	return nullptr;
}

bool FlowLayout::hasHeightForWidth() const {
	return true;
}


//int FlowLayout::heightForWidth(const int width) const {
//	const int height = do_layout(QRect(0, 0, width, 0), true);
//	return height;
//}

int FlowLayout::heightForWidth(const int width) const {
	const int height = do_layout(QRect(0, 0, width, 0), true, nullptr); // jpo38: set added parameter to NULL here
	return height;
}

//void FlowLayout::setGeometry(const QRect &rect) {
//	QLayout::setGeometry(rect);
//	do_layout(rect, false);
//}

void FlowLayout::setGeometry(const QRect &rect) {
	QLayout::setGeometry(rect);

	const QSize oldSize = min_size;
	do_layout(rect, false, &min_size);
	if (oldSize != min_size) {
		// force layout to consider new minimum size!
		invalidate();
	}
}

QSize FlowLayout::sizeHint() const {
	return minimumSize();
}

//QSize FlowLayout::minimumSize() const {
//	QSize size;
//	for (auto&& item : item_list) {
//		size = size.expandedTo(item->minimumSize());
//	}
//
//	size += QSize(2 * margin(), 2 * margin());
//	return size;
//}

QSize FlowLayout::minimumSize() const {
	return min_size;
}

QList<QLayoutItem*> FlowLayout::items() const {
	return item_list;
}

void FlowLayout::clear() {
	for (auto&& i : item_list) {
		i->widget()->deleteLater();
		delete i;
	}
	item_list.clear();
}

//int FlowLayout::do_layout(const QRect &rect, const bool test_only) const {
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
//}

int FlowLayout::do_layout(const QRect &rect, const bool test_only, QSize* p_min_size) const {
	int left, top, right, bottom;
	getContentsMargins(&left, &top, &right, &bottom);
	QRect effective_rect = rect.adjusted(+left, +top, -right, -bottom);
	int x = effective_rect.x();
	int y = effective_rect.y();
	int line_height = 0;

	// jpo38: store max X
	int max_x = 0;

	for (auto&& item : item_list) {
		QWidget *wid = item->widget();
		int space_x = horizontal_spacing();
		if (space_x == -1)
			space_x = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
		int space_y = vertical_spacing();
		if (space_y == -1)
			space_y = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
		int next_x = x + item->sizeHint().width() + space_x;
		if (next_x - space_x > effective_rect.right() && line_height > 0) {
			x = effective_rect.x();
			y = y + line_height + space_y;
			next_x = x + item->sizeHint().width() + space_x;
			line_height = 0;
		}

		if (!test_only)
			item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

		// jpo38: update max X based on current position
		max_x = qMax(max_x, x + item->sizeHint().width() - rect.x() + left);

		x = next_x;
		line_height = qMax(line_height, item->sizeHint().height());
	}

	// jpo38: save height/width as max height/xidth in p_min_size is specified
	const int height = y + line_height - rect.y() + bottom;
	if (p_min_size) {
		p_min_size->setHeight(height);
		p_min_size->setWidth(max_x);
	}
	return height;
}

int FlowLayout::smart_spacing(const QStyle::PixelMetric pm) const {
	QObject *parent = this->parent();
	if (!parent) {
		return -1;
	} else if (parent->isWidgetType()) {
		QWidget *pw = dynamic_cast<QWidget *>(parent);
		return pw->style()->pixelMetric(pm, nullptr, pw);
	} else {
		return dynamic_cast<QLayout *>(parent)->spacing();
	}
}