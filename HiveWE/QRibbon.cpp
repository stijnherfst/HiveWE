#include "stdafx.h"

QRibbonButton::QRibbonButton(QWidget* parent) : QToolButton(parent) {
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

	setStyleSheet(R"(
		QToolButton {
			border: 1px solid transparent;
			padding-left: 3px;
			padding-right: 2px;
		}

		QToolButton::hover {
			border: 1px solid rgb(164, 206, 249);
			background-color: rgb(232, 239, 247);
		}

		QToolButton::pressed {
			border: 1px solid rgb(98, 162, 228);
			background-color: rgb(201, 224, 247);
		}

		QToolButton::checked {
			border: 1px solid rgb(98, 162, 228);
			background-color: rgb(201, 224, 247);
		}
	)");

	setIconSize({ 32, 32 });
	setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
}

QRibbonSection::QRibbonSection(QWidget* parent) : QWidget(parent) {
	layouttt->setContentsMargins(0, 0, 0, 0);
	layoutt->setContentsMargins(0, 0, 0, 0);
	layoutt->setSpacing(0);
	setLayout(layouttt);
	layout()->setContentsMargins(0, 0, 0, 0);
	layouttt->addLayout(layoutt);
	layouttt->addWidget(section_text);
	setContentsMargins(0, 0, 0, 0);
	layouttt->setMargin(0);
	layoutt->setMargin(0);

	section_text->setAlignment(Qt::AlignBottom | Qt::AlignmentFlag::AlignHCenter);
}

void QRibbonSection::addWidget(QWidget* widget) {
	layoutt->addWidget(widget);
}

void QRibbonSection::addLayout(QLayout* layout) {
	layoutt->addLayout(layout);
}


void QRibbonSection::setText(const QString& text) {
	section_text->setText(text);
}

QRibbonTab::QRibbonTab(QWidget* parent) : QWidget(parent) {
	setContentsMargins(0, 0, 0, 0);
	sections->addStretch(1);
	layout()->setContentsMargins(6, 4, 5, 3);

	setStyleSheet(R"(
		QRibbonTab {
			background-color: rgb(245, 246, 247);
		}
	)");
}

void QRibbonTab::paintEvent(QPaintEvent* event) {
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

	QWidget::paintEvent(event);
}

void QRibbonTab::add_section(QLayout* layout) {
	sections->insertLayout(0, layout);
}

void QRibbonTab::add_section(QRibbonSection* section) {
	QFrame* line = new QFrame();
	line->setFrameShape(QFrame::VLine);
	line->setFrameShadow(QFrame::Sunken);

	line->setStyleSheet(R"(
		QFrame{
			border: none;
			max-width: 1px;
			background-color: rgb(226, 227, 228);
		}
	)");

	int count = sections->count();
	sections->insertWidget(count - 1, section);
	sections->insertWidget(count, line);
}


QRibbonMenu::QRibbonMenu(QWidget* parent) : QMenu(parent) {
	setStyleSheet(R"(
		QMenu {
			border: 1px solid rgb(132, 146, 166);
		}
		QToolButton {
			border: 1px solid transparent;
			background-color: rgb(251, 252, 253);
			height: 44px;
			width: 216px;
		}

		QToolButton::hover {
			border-color: rgb(168, 210, 253);
			background-color: rgb(237, 244, 252);
		}
	)");

	recent_maps->setStyleSheet(R"(
		QListWidget {
			border: none;
			font: 8pt "Segoe UI";
			background-color: rgb(246, 247, 248);
			outline: 0;
		}

		QListView::item {
			border: 1px solid transparent;
		}

		QListView::item::hover {
			background-color: rgb(233, 240, 248);
			border-color: rgb(165, 207, 249);
		}

		QListView::item:pressed {
			background-color: rgb(233, 240, 248);
			border-color: rgb(0, 207, 249);
		}

		QListView::item:selected {
			background-color: rgb(233, 240, 248);
			border-color: rgb(0, 207, 249);
		}

		QListView::item:selected:active {
			background-color: rgb(233, 240, 248);
			border-color: rgb(0, 207, 249);
		}

		QListView::item:selected:!active {
			background-color: rgb(233, 240, 248);
			border-color: rgb(0, 207, 249);
		}

		QListView::text {
			left: 8px;
		}
	)");

	recent_maps->setMinimumWidth(300);

	base->setContentsMargins(1, 1, 1, 1);
	base->setSpacing(0);
	actions->setContentsMargins(0, 0, 0, 0);

	QFrame* line = new QFrame();
	line->setFrameShape(QFrame::VLine);
	line->setFrameShadow(QFrame::Plain);
	line->setStyleSheet(R"(
		QFrame {
			border: none;
			max-width: 0x;
			border-left: 1px solid rgb(220, 221, 222);
			border-right: 1px solid rgb(254, 254, 255);
		}
	)");

	base->addLayout(actions);
	base->addWidget(line);
	base->addLayout(frequent_places);

	setLayout(base);

	frequent_places->setContentsMargins(0, 0, 0, 0);

	QLabel* label = new QLabel("Recent Maps");
	label->setStyleSheet(R"(
		QLabel {
			background-color: rgb(246, 247, 248);
			
		}
	)");

	frequent_places->addWidget(label);
	frequent_places->addWidget(recent_maps);
	recent_maps->addItem("MCFC 7.0");
}

QRibbonFileButton::QRibbonFileButton(QWidget* parent) : QToolButton(parent) {
	setStyleSheet(R"(
		QToolButton {
			border: 1px solid transparent;
			background-color: rgb(25, 121, 202);
			color: rgb(255, 255, 255);
			height: 23px;
			padding-left: 14px;
			padding-right: 13px;
		}

		QToolButton::hover {
			background-color: rgb(41, 140, 225);
		}

		QToolButton::pressed {
			background-color: rgb(18, 104, 179);
		}

		QToolButton::menu-indicator { 
			image: none; 
		}
	)");

	setText("File");
	setMenu(menu);
	setPopupMode(QToolButton::InstantPopup);
}

QRibbon::QRibbon(QWidget *parent) : QTabWidget(parent) {
	setStyleSheet(R"(
		QWidget {
			font: 8pt "Segoe UI";
		}

		QTabWidget::tab-bar {
			color: rgb(255, 0, 174);
		}

		QTabWidget::pane {
			border-top: 1px solid rgb(218, 219, 220);
			border-bottom: 1px solid rgb(218, 219, 220);
			margin-top: -1;
		}

		QTabBar::tab { 
			height: 23px;
			background-color: rgb(255, 255, 255);
			border: 1px solid rgb(255, 255, 255);
			border-bottom-width: 0px;
			padding-left: 13px;
			padding-right: 13px;
			margin-right: 1px;
		}

		QTabBar::tab:hover {
			background-color: rgb(253, 253, 255);
			border-color: rgb(235, 236, 236);
			border-bottom-width: 0;
		}

		QTabBar::tab:selected {
			background-color: rgb(245, 246, 247);
			border-color: rgb(218, 219, 220);
			border-bottom-width: 0;
		}

		QTabBar::tab::!selected {
			border-bottom: 1px solid rgb(218, 219, 220);
			padding-bottom: -1;
		}
	)");

	setCornerWidget(file, Qt::TopLeftCorner);
}

void QRibbon::addMenuItem(QWidget* widget) {
	file->menu->actions->addWidget(widget);
}

void QRibbon::addMenuSeperator() {
	QFrame* line = new QFrame();
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Plain);
	line->setStyleSheet(R"(
		QFrame {
			background-color: rgb(220, 221, 222);
			border: none;
			max-height: 1px;
			border-left: 44px solid rgb(251, 252, 253);
		}
	)");
	file->menu->actions->addWidget(line);
}

QRibbon::~QRibbon() {
}