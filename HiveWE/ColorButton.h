#pragma once

class ColorButton : public QPushButton {
	Q_OBJECT
public:
	ColorButton(QWidget* parent);

	void setColor(const QColor& color);
	const QColor& getColor();
	glm::vec4 get_glm_color() const;

public slots:
	void changeColor();

private:
	QColor color;
};