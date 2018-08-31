#pragma once

class QRibbonButton : public QToolButton {
	Q_OBJECT

public:
	QRibbonButton(QWidget* parent = nullptr);
};

class QRibbonSection : public QWidget {
	Q_OBJECT

public:
	QRibbonSection(QWidget* parent = nullptr);

	QLabel* section_text = new QLabel;
	QVBoxLayout* layouttt = new QVBoxLayout;
	QHBoxLayout* layoutt = new QHBoxLayout;

	void addWidget(QWidget* widget);
	void addLayout(QLayout* layout);

	void setText(const QString& text);
};

class QRibbonTab : public QWidget {
	Q_OBJECT

public:
	QRibbonTab(QWidget* parent = nullptr);
	QHBoxLayout* sections = new QHBoxLayout(this);
	void add_section(QLayout* layout);
	void add_section(QRibbonSection* layout);

	void paintEvent(QPaintEvent* event);
};

class QRibbon : public QTabWidget {
	Q_OBJECT

	QToolButton* file = new QToolButton;

public:
	QRibbon(QWidget *parent = nullptr);
	~QRibbon();
};
