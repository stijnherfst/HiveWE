#include "IconView.h"
#include "HiveWE.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QButtonGroup>

#include "FlowLayout.h"

IconView::IconView(QWidget* parent) : QWidget(parent) {
	FlowLayout* flow_layout = new FlowLayout;
	flow_layout->setSpacing(3);
	flow_layout->setMargin(0);

	QWidget* widget = new QWidget;
	widget->setLayout(flow_layout);

	QScrollArea* scroll_area = new QScrollArea;
	scroll_area->setWidgetResizable(true);
	scroll_area->setWidget(widget);

	QVBoxLayout* layout = new QVBoxLayout;
	layout->setMargin(0);
	layout->addWidget(type);
	layout->addWidget(search);
	layout->addWidget(scroll_area);
	setLayout(layout);

	type->addItem("Units");
	type->addItem("Items");
	type->addItem("Abilities");
	type->addItem("Upgrades");
	type->addItem("Buffs");
	search->setPlaceholderText("Search Icons");

	for (const auto& [key, values] : units_slk.base_data) {
		if (values.contains("art")) {
			if (icons.contains(values.at("art"))) {
				QString& data = icons.at(values.at("art"));
				QString new_data = QString::fromStdString(values.at("name"));
				if (!data.contains(new_data)) {
					data += new_data;
					if (data.size()) {
						data += "\n";
					}
				}
			} else {
				icons.emplace(values.at("art"), QString::fromStdString(values.at("name")));
			}
		}
	}

	QButtonGroup* group = new QButtonGroup(this);
	group->setExclusive(true);

	for (const auto& [key, value] : icons) {
		IconButton* button = new IconButton;
		button->setCheckable(true);

		QLabel* pIconLabel = new QLabel;
		auto icon = resource_manager.load<QIconResource>(key);
		pIconLabel->setPixmap(icon->icon.pixmap(64, 64));
		pIconLabel->setAlignment(Qt::AlignCenter);
		pIconLabel->setMouseTracking(false);

		QLabel* pTextLabel = new QLabel;
		pTextLabel->setText(value);
		pTextLabel->setAlignment(Qt::AlignCenter);
		pTextLabel->setWordWrap(true);
		pTextLabel->setTextInteractionFlags(Qt::NoTextInteraction);
		pTextLabel->setMouseTracking(false);
		pTextLabel->setMaximumWidth(64);
		pTextLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

		QVBoxLayout* pLayout = new QVBoxLayout;
		pLayout->addWidget(pIconLabel);
		pLayout->addWidget(pTextLabel);
		pLayout->setSpacing(5);
		pLayout->setMargin(0);
		pLayout->setContentsMargins(3, 3, 3, 3);

		group->addButton(button);
		button->setLayout(pLayout);
		flow_layout->addWidget(button);
	}
}