#pragma once
#include <QLabel>

class AspectRatioPixmapLabel : public QLabel {
	Q_OBJECT

	QPixmap pixmap;

	QPixmap get_scaled_pixmap(bool grid_lines = false);

public:
	AspectRatioPixmapLabel() {
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	}
	using QLabel::QLabel;

	int horizontal_border;
	int vertical_border;

public slots:
	void setPixmap(const QPixmap&);
	void resizeEvent(QResizeEvent*) override;
};