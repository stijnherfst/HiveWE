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


class QRibbonMenu : public QMenu {
	Q_OBJECT
public:
	QHBoxLayout* base = new QHBoxLayout;
	QVBoxLayout* actions = new QVBoxLayout;
	QVBoxLayout* frequent_places = new QVBoxLayout;

	QListWidget* recent_maps = new QListWidget;

	QRibbonMenu(QWidget* parent = nullptr);
};

class QRibbonFileButton : public QToolButton {
	Q_OBJECT
public:
	QRibbonMenu* menu = new QRibbonMenu;
	QRibbonFileButton(QWidget* parent = nullptr);
};

class QRibbon : public QTabWidget {
	Q_OBJECT

	QRibbonFileButton* file = new QRibbonFileButton;

public:
	void addMenuItem(QWidget* widget);
	void addMenuSeperator();

	QRibbon(QWidget *parent = nullptr);
	~QRibbon();
};
