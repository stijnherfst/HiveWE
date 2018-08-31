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

	int count = sections->count();
	sections->insertWidget(count - 1, section);
	sections->insertWidget(count, line);
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

	file->setText("File");
	setCornerWidget(file, Qt::TopLeftCorner);

	QMenu* test = new QMenu;
	QWidgetAction* w = new QWidgetAction(test);

	file->setStyleSheet(R"(
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
	)");


	/*QFont font;
	font.setFamily("Segoe UI");

	setFont(font);*/
}

QRibbon::~QRibbon() {
}