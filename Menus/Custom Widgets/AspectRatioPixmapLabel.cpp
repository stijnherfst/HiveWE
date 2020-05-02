#include "AspectRatioPixmapLabel.h"

#include <QPainter>
#include <QPixmap>

QPixmap AspectRatioPixmapLabel::get_scaled_pixmap(bool grid_lines) {
	QPixmap new_pixmap(width(), height());

	QPainter painter(&new_pixmap);
	painter.fillRect(0, 0, width(), height(), Qt::black);

	QPixmap scaled_pixmap = pixmap.scaled(width(), height(), Qt::KeepAspectRatio);
	horizontal_border = (width() - scaled_pixmap.width()) / 2.f;
	vertical_border = (height() - scaled_pixmap.height()) / 2.f;

	painter.drawPixmap(horizontal_border, vertical_border, scaled_pixmap);


	/*if (grid_lines) {
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
	}*/

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