#pragma once

class AspectRatioPixmapLabel : public QLabel {
	Q_OBJECT


	QPixmap pixmap;

	QPixmap get_scaled_pixmap(bool grid_lines = false);

public:
	AspectRatioPixmapLabel() {
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	}
	using QLabel::QLabel;

public slots:
	void setPixmap(const QPixmap&);
	void resizeEvent(QResizeEvent*) override;
};