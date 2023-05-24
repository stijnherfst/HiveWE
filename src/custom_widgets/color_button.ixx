module;

#include <QPushButton>
#include <QColorDialog>
#include <QColor>

#define GLM_FORCE_CXX17
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SILENT_WARNINGS
#include <glm/glm.hpp>

export module ColorButton;

export class ColorButton : public QPushButton {
	Q_OBJECT
  public:
	explicit ColorButton(QWidget* parent)
		: QPushButton(parent) {
		connect(this, &QPushButton::clicked, this, &ColorButton::changeColor);
	}

	void setColor(const QColor& new_color) {
		color = new_color;

		int delta = color.red() * 0.299 + color.green() * 0.587 + color.blue() * 0.114;
		QColor text_color = QColor((255 - delta < 105) ? Qt::black : Qt::white);

		setStyleSheet("border-color: " + color.name() + "; " + QString("background-color: ") + color.name() + "; color: " + text_color.name());
	}

	const QColor& getColor() {
		return color;
	}

	glm::vec4 get_glm_color() const {
		return { color.red(), color.green(), color.blue(), color.alpha() };
	}

  public slots:
	void changeColor() {
		QColor newColor = QColorDialog::getColor(color, parentWidget());
		if (newColor != color) {
			setColor(newColor);
		}
	}

  private:
	QColor color;
};

#include "color_button.moc"