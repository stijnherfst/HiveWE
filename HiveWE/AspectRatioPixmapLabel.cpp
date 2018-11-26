#include "stdafx.h"

QPixmap AspectRatioPixmapLabel::get_scaled_pixmap(bool grid_lines) {
	//auto t = height();
	//auto tt = pixmap.height();
	int new_height = pixmap.height() * ((float)height() / pixmap.height());

	QPixmap new_pixmap = pixmap.scaled(999, new_height, Qt::KeepAspectRatio);

	if (grid_lines) {
		QPainter painter(&new_pixmap);
		auto tt = new_pixmap.width();
		auto ttt = pixmap.width();

		const int horizontal_spacing = new_pixmap.width() / pixmap.width();
		const int vertical_spacing = new_pixmap.height() / pixmap.height();

		if (horizontal_spacing > 2) {
			for (int i = 1; i < pixmap.width(); i++) {
				painter.drawLine(i * horizontal_spacing, 0, i * horizontal_spacing, new_pixmap.height());
			}
		}

		if (vertical_spacing > 2) {
			for (int j = 1; j < pixmap.height(); j++) {
				painter.drawLine(0, vertical_spacing * j, new_pixmap.width(), vertical_spacing * j);
			}
		}
		painter.end();
	}

	return new_pixmap;
}

void AspectRatioPixmapLabel::setPixmap(const QPixmap& p) {
	pixmap = p;

	if (pixmap.isNull()) {
		return;
	}

	QLabel::setPixmap(get_scaled_pixmap(false));
}

void AspectRatioPixmapLabel::resizeEvent(QResizeEvent* e) {
	if (pixmap.isNull()) {
		return;
	}

	QLabel::setPixmap(get_scaled_pixmap(false));
}