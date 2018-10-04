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

QRibbonButton::~QRibbonButton() {
	for (auto&& i : shortcuts) {
		i->setParent(nullptr);
		delete i;
	}
}

void QRibbonButton::setShortCut(const QKeySequence sequence, const std::vector<QWidget*>& attach_to) {
	for (auto&& i : attach_to) {
		shortcuts.push_back(new QShortcut(sequence, i));
		connect(shortcuts.back(), &QShortcut::activated, this, &QToolButton::click);
		connect(shortcuts.back(), &QShortcut::activatedAmbiguously, []() { printf("kut2\n"); });
	}
}

void QRibbonButton::disableShortcuts() {
	for (auto&& i : shortcuts) {
		i->setEnabled(false);
	}
}

void QRibbonButton::enableShortcuts() {
	for (auto&& i : shortcuts) {
		i->setEnabled(true);
	}
}

void QRibbonButton::disconnectShortcuts() {
	//for (auto&& i : shortcuts) {
	//	delete i;
	//	shortcuts.clear();
	//}
}

QRibbonContainer::QRibbonContainer(QWidget* parent) : QFrame(parent) {
	setLayout(layout);
	setAutoFillBackground(true);
	setStyleSheet(R"(
		QFrame {
			border: 1px solid rgb(219, 220, 221);
			background-color: rgb(250, 251, 252);
		}
	)");

	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
}

void QRibbonContainer::addWidget(QWidget* widget, int row, int column) {
	layout->addWidget(widget, row, column);
}

void QRibbonContainer::clear() {
	QLayoutItem *item;
	while ((item = layout->takeAt(0))) {
		if (item->widget()) {
			delete item->widget();
		}
		delete item;
	}
}

QRibbonSection::QRibbonSection(QWidget* parent) : QWidget(parent) {
	setLayout(section_inner);

	section_inner->setContentsMargins(0, 0, 0, 0);
	section_inner->setSpacing(5);
	section_inner->setMargin(0);

	section_outer->setContentsMargins(0, 0, 0, 0);
	section_outer->setSpacing(0);
	section_outer->setMargin(0);

	layout()->setContentsMargins(0, 0, 0, 0);
	setContentsMargins(0, 0, 0, 0);
	
	section_inner->addLayout(section_outer);
	section_inner->addWidget(section_text);

	section_text->setAlignment(Qt::AlignBottom | Qt::AlignmentFlag::AlignHCenter);
}

void QRibbonSection::addWidget(QWidget* widget) {
	section_outer->addWidget(widget);
}

void QRibbonSection::addLayout(QLayout* layout) {
	section_outer->addLayout(layout);
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

void QRibbonTab::disableShortcuts() {
	
}

void QRibbonTab::enableShortcuts() {

}

void QRibbonTab::paintEvent(QPaintEvent* event) {
	QStyleOption opt;
	opt.init(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

	QWidget::paintEvent(event);
}

void QRibbonTab::addSection(QLayout* layout) {
	sections->insertLayout(0, layout);
}

void QRibbonTab::addSection(QRibbonSection* section) {
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

	//QLabel* label = new QLabel("Recent Maps");
	//label->setStyleSheet(R"(
	//	QLabel {
	//		background-color: rgb(246, 247, 248);
	//		
	//	}
	//)");

	//frequent_places->addWidget(label);
	
	//for (int i = 0; i < 5; i++) {
	//	QLabel* labell = new QLabel("test");
	//	labell->setStyleSheet(R"(
	//		QLabel::hover {
	//			background-color: rgb(233, 240, 248);
	//			border: 1px solid rgb(165, 207, 249);
	//			
	//		}
	//	)");
	//	frequent_places->addWidget(labell);
	//}
	//frequent_places->addStretch(1);
}

//bool QRibbonMenu::eventFilter(QObject *obj, QEvent *event) {
//	if (event->type() == QEvent::KeyPress) {
//		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
//		qDebug("Ate key press %d", keyEvent->key());
//		return true;
//	} else {
//		return QObject::eventFilter(obj, event);
//	}
//}

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

void QRibbon::addMenuItem(QAbstractButton* widget) {
	file->menu->actions->addWidget(widget);
	connect(widget, &QAbstractButton::clicked, [&]() { file->menu->close(); });

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