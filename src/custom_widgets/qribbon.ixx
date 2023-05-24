module;

#include <vector>

#include <QShortcut>
#include <QToolButton>
#include <QFrame>
#include <QGridLayout>
#include <QBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QTabWidget>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <QMap>
#include <QScrollArea>
#include <QPushButton>
#include <QKeySequence>

export module QRibbon;

export class QRibbonButton : public QToolButton {
	Q_OBJECT

  public:
	QRibbonButton(QWidget* parent = nullptr)
		: QToolButton(parent) {
		setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	}

	~QRibbonButton() {
		for (auto&& i : shortcuts) {
			i->setParent(nullptr);
			delete i;
		}
	}

	std::vector<QShortcut*> shortcuts;

	void setShortCut(const QKeySequence sequence, const std::vector<QWidget*>& attach_to) {
		for (auto&& i : attach_to) {
			shortcuts.push_back(new QShortcut(sequence, i));
			connect(shortcuts.back(), &QShortcut::activated, this, &QToolButton::click);
			connect(shortcuts.back(), &QShortcut::activatedAmbiguously, []() { printf("kut2\n"); });
		}
	}

	void disableShortcuts() {
		for (auto&& i : shortcuts) {
			i->setEnabled(false);
		}
	}

	void enableShortcuts() {
		for (auto&& i : shortcuts) {
			i->setEnabled(true);
		}
	}

	void disconnectShortcuts() {
		// for (auto&& i : shortcuts) {
		//	delete i;
		//	shortcuts.clear();
		// }
	}
};

export class QSmallRibbonButton : public QRibbonButton {
	Q_OBJECT

  public:
	using QRibbonButton::QRibbonButton;

	// QSmallRibbonButton(QWidget* parent = nullptr);
	//~QSmallRibbonButton();
};

export class QRibbonContainer : public QFrame {
	Q_OBJECT

	QGridLayout* layout = new QGridLayout;

  public:
	explicit QRibbonContainer(QWidget* parent = nullptr)
		: QFrame(parent) {
		setLayout(layout);
		setAutoFillBackground(true);

		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);
	}

	void addWidget(QWidget* widget, int row, int column) {
		layout->addWidget(widget, row, column);
	}

	void clear() {
		QLayoutItem* item;
		while ((item = layout->takeAt(0))) {
			if (item->widget()) {
				delete item->widget();
			}
			delete item;
		}
	}
};

export class QRibbonSection : public QWidget {
	Q_OBJECT

  public:
	QLabel* section_text = new QLabel;
	QVBoxLayout* section_inner = new QVBoxLayout;
	QHBoxLayout* section_outer = new QHBoxLayout;

	explicit QRibbonSection(QWidget* parent = nullptr)
		: QWidget(parent) {
		setLayout(section_inner);

		section_inner->setContentsMargins(0, 0, 0, 0);
		section_inner->setSpacing(5);
		section_inner->setContentsMargins(0, 0, 0, 0);

		section_outer->setContentsMargins(0, 0, 0, 0);
		section_outer->setSpacing(0);
		section_outer->setContentsMargins(0, 0, 0, 0);

		layout()->setContentsMargins(0, 0, 0, 0);
		setContentsMargins(0, 0, 0, 0);

		section_inner->addLayout(section_outer);
		section_inner->addWidget(section_text);

		section_text->setAlignment(Qt::AlignBottom | Qt::AlignmentFlag::AlignHCenter);
	}

	void addWidget(QWidget* widget) {
		section_outer->addWidget(widget);
	}

	void addLayout(QLayout* layout) {
		section_outer->addLayout(layout);
	}

	void addSpacing(int spacing) {
		section_outer->addSpacing(spacing);
	}

	void setText(const QString& text) {
		section_text->setText(text);
	}
};

export class QRibbonTab : public QWidget {
	Q_OBJECT

  public:
	QHBoxLayout* sections = new QHBoxLayout(this);

	explicit QRibbonTab(QWidget* parent = nullptr)
		: QWidget(parent) {
		setContentsMargins(0, 0, 0, 0);
		sections->addStretch(1);
		layout()->setContentsMargins(6, 4, 5, 3);
	}

	void disableShortcuts() {
	}

	void enableShortcuts() {
	}

	void paintEvent(QPaintEvent* event) override {
		QStyleOption opt;
		opt.initFrom(this);
		QPainter p(this);
		style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

		QWidget::paintEvent(event);
	}

	void addSection(QLayout* layout) {
		sections->insertLayout(0, layout);
	}

	void addSection(QRibbonSection* section) {
		QFrame* line = new QFrame();
		line->setObjectName("seperator");

		int count = sections->count();
		sections->insertWidget(count - 1, section);
		sections->insertWidget(count, line);
	}

	void addSpacer(QSpacerItem* spacer) {
		QFrame* line = new QFrame();
		line->setObjectName("seperator");

		int count = sections->count();
		sections->insertSpacerItem(count - 1, spacer);
		sections->insertWidget(count, line);
	}
};

export class QRibbonMenu : public QMenu {
	Q_OBJECT
  public:
	QHBoxLayout* base = new QHBoxLayout;
	QVBoxLayout* actions = new QVBoxLayout;
	QVBoxLayout* frequent_places = new QVBoxLayout;

	explicit QRibbonMenu(QWidget* parent = nullptr)
		: QMenu(parent) {
		base->setContentsMargins(1, 1, 1, 1);
		base->setSpacing(0);
		actions->setContentsMargins(0, 0, 0, 0);

		QFrame* line = new QFrame();
		line->setObjectName("verticalSeperator");

		base->addLayout(actions);
		base->addWidget(line);
		// base->addLayout(frequent_places);

		setLayout(base);

		// frequent_places->setContentsMargins(0, 0, 0, 0);

		// QLabel* label = new QLabel("Recent Maps");
		// label->setStyleSheet(R"(
		//	QLabel {
		//		background-color: rgb(246, 247, 248);
		//
		//	}
		//)");

		// frequent_places->addWidget(label);

		// for (int i = 0; i < 5; i++) {
		//	QLabel* labell = new QLabel("test");
		//	labell->setStyleSheet(R"(
		//		QLabel::hover {
		//			background-color: rgb(233, 240, 248);
		//			border: 1px solid rgb(165, 207, 249);
		//
		//		}
		//	)");
		//	frequent_places->addWidget(labell);
		// }
		// frequent_places->addStretch(1);
	}

	// bool eventFilter(QObject *obj, QEvent *event);
};

export class QRibbonFileButton : public QToolButton {
	Q_OBJECT
  public:
	QRibbonMenu* menu = new QRibbonMenu;
	explicit QRibbonFileButton(QWidget* parent = nullptr)
		: QToolButton(parent) {
		setText("File");
		setMenu(menu);
	}
};

export class QRibbon : public QTabWidget {
	Q_OBJECT

	QRibbonFileButton* file = new QRibbonFileButton;

  public:
	explicit QRibbon(QWidget* parent = nullptr)
		: QTabWidget(parent) {
		setCornerWidget(file, Qt::TopLeftCorner);
	}

	void addMenuItem(QAbstractButton* widget) {
		file->menu->actions->addWidget(widget);
		connect(widget, &QAbstractButton::clicked, [&]() { file->menu->close(); });
	}

	void addMenuSeperator() {
		QFrame* line = new QFrame();
		line->setObjectName("horizontalSeperator");
		file->menu->actions->addWidget(line);
	}

	~QRibbon() {
	}
};

#include "qribbon.moc"