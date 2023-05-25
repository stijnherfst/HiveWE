
#include "icon_view.h"
#include "globals.h"

#include <fstream>
#include <print>

#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QListView>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>

#include <filesystem>
namespace fs = std::filesystem;

import Hierarchy;

std::unordered_map<std::string, std::shared_ptr<QIconResource>> icon_cache;

IconModel::IconModel(QObject* parent) : QAbstractListModel(parent) {
	//std::unordered_map<std::string, QString> icons_map;

	//for (const auto& [key, values] : units_slk.base_data) {
	//	if (!values.contains("art")) {
	//		continue;
	//	}

	//	//std::string art_path = values.at("art");
	//	std::string art_path = to_lowercase_copy(values.at("art"));
	//	art_path = fs::path(art_path).replace_extension("").string();
	//	art_path = split(art_path, ',').front();

	//	if (icons_map.contains(art_path)) {
	//		QString& data = icons_map.at(art_path);
	//		QString new_data = QString::fromStdString(values.at("name"));
	//		if (!data.contains(new_data)) {
	//			data += ", " + new_data;
	//		}
	//	} else {
	//		icons_map.emplace(art_path, QString::fromStdString(values.at("name")));
	//	}
	//}

	//for (const auto& [key, values] : items_slk.base_data) {
	//	if (!values.contains("art")) {
	//		continue;
	//	}

	//	//std::string art_path = values.at("art");
	//	std::string art_path = to_lowercase_copy(values.at("art"));
	//	art_path = fs::path(art_path).replace_extension("").string();
	//	art_path = split(art_path, ',').front();

	//	if (icons_map.contains(art_path)) {
	//		QString& data = icons_map.at(art_path);
	//		QString new_data = QString::fromStdString(values.at("name"));
	//		if (!data.contains(new_data)) {
	//			data += ", " + new_data;
	//		}
	//	} else {
	//		icons_map.emplace(art_path, QString::fromStdString(values.at("name")));
	//	}
	//}

	//for (const auto& [key, values] : buff_slk.base_data) {
	//	if (!values.contains("buffart")) {
	//		continue;
	//	}

	//	QString tags;
	//	if (!values.contains("bufftip") && !values.contains("editorname")) {
	//		std::print("Missing buff name: {}\n", key);
	//		continue;
	//	} else if (!values.contains("bufftip")) {
	//		tags = QString::fromStdString(values.at("editorname"));
	//	} else {
	//		tags = QString::fromStdString(values.at("bufftip"));
	//	}
	//	
	//	//std::string art_path = values.at("buffart");
	//	std::string art_path = to_lowercase_copy(values.at("buffart"));
	//	art_path = fs::path(art_path).replace_extension("").string();
	//	art_path = split(art_path, ',').front();

	//	if (icons_map.contains(art_path)) {
	//		QString& data = icons_map.at(art_path);

	//		if (!data.contains(tags)) {
	//			data += ", " + tags;
	//		}
	//	} else {
	//		icons_map.emplace(art_path, tags);
	//	}
	//}

	//for (const auto& [key, values] : abilities_slk.base_data) {
	//	if (!values.contains("art")) {
	//		continue;
	//	}

	//	QString tags;
	//	if (values.contains("name")) {
	//		tags = QString::fromStdString(values.at("name"));
	//	} else if (values.contains("tip")) {
	//		tags = QString::fromStdString(values.at("tip"));
	//	} else {
	//		std::print("Missing ability name: {}\n", key);
	//		continue;
	//	}
	//	std::string art_path = to_lowercase_copy(values.at("art"));
	//	art_path = fs::path(art_path).replace_extension("").string();

	//	//std::string art_path = values.at("art");
	//	art_path = split(art_path, ',').front();

	//	if (icons_map.contains(art_path)) {
	//		QString& data = icons_map.at(art_path);
	//		if (!data.contains(tags)) {
	//			data += ", " + tags;
	//		}
	//	} else {
	//		icons_map.emplace(art_path, tags);
	//	}
	//}

	//for (const auto& i : fs::directory_iterator("C:/Users/User/Desktop/Warcraft/MPQContent/1.32.x/_hd.w3mod/replaceabletextures/commandbuttons")) {
	//	fs::path path = i.path().lexically_relative("C:/Users/User/Desktop/Warcraft/MPQContent/1.32.x/_hd.w3mod");
	//	path.replace_extension("");
	//	if (icons_map.contains(path.string())) {
	//		continue;
	//	}

	//	icons_map.emplace(path.string(), QString::fromStdString(path.stem().string()).remove(0, 3));
	//}

	//QJsonDocument json;
	//QJsonArray array;
	//for (const auto& [key, value] : icons_map) {
	//	icons.push_back({ key, value });
	//	QJsonObject object;
	//	object["src"] = QString::fromStdString(key);
	//	
	//	QJsonArray raaaa;
	//	auto parts = value.split(", ");
	//	for (const auto& i : parts) {
	//		raaaa.append(i);
	//	}
	//	object["tags"] = raaaa;
	//
	//	array.append(object);
	//}
	//json.setArray(array);
	//std::ofstream file("C:/Users/User/stack/Projects/HiveWE/HiveWE/Data/Warcraft/icon_tags.json");
	//std::string output = json.toJson().toStdString();
	//file.write(output.data(), output.size());
	//file.close();

	QFile file(fs::path("Data/Warcraft/icon_tags.json"));
	file.open(QIODevice::ReadOnly);

	QJsonParseError error;
	QJsonDocument json = QJsonDocument::fromJson(file.readAll(), &error);
	file.close();

	if (json.isNull()) {
		std::print("Error parsing icon_tags.json: {}", error.errorString().toStdString());
	}

	for (const auto& i : json.array()) {
		QJsonObject object = i.toObject();
		
		QJsonArray tags = object["tags"].toArray();

		QString text;
		if (tags.size()) {
			text = tags.at(0).toString();
			for (int i = 1; i < tags.size(); i++) {
				text += ", " + tags.at(i).toString();
			}
		}

		std::string string_path = object["src"].toString().toStdString() + ".dds";
		if (!hierarchy.file_exists(string_path)) {
			continue;
		}
		icons.emplace_back(string_path, text);
	}
}

QVariant IconModel::data(const QModelIndex& index, int role) const {
	switch (role) {
		case Qt::DecorationRole: {
			std::string string_path = icons[index.row()].first;

			if (icon_cache.contains(string_path)) {
				return icon_cache.at(string_path)->icon;
			} else {
				icon_cache.emplace(string_path, resource_manager.load<QIconResource>(string_path));
				return icon_cache.at(string_path)->icon;
			}
		}
		case Qt::ToolTipRole:
			return icons[index.row()].second;
		case Qt::EditRole:
			return QString::fromStdString(icons[index.row()].first);
	}

	return {};
}

IconView::IconView(QWidget* parent) : QWidget(parent) {
	model = new IconModel;

	filter->setSourceModel(model);
	filter->setFilterCaseSensitivity(Qt::CaseInsensitive);
	filter->setFilterRole(Qt::ItemDataRole::ToolTipRole);

	view->setViewMode(QListView::IconMode);
	view->setIconSize(QSize(64, 64));
	view->setResizeMode(QListView::ResizeMode::Adjust);
	view->setUniformItemSizes(true);
	view->setWordWrap(true);
	view->setWrapping(true);
	view->setModel(filter);

	QVBoxLayout* layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	//layout->addWidget(type);
	layout->addWidget(search);
	layout->addWidget(view);

	QHBoxLayout* hlayout = new QHBoxLayout;
	hlayout->addWidget(new QLabel("Path"));
	hlayout->addWidget(finalPath);
	//layout->addWidget(finalPath);
	layout->addLayout(hlayout);
	setLayout(layout);

	type->addItem("Units");
	type->addItem("Items");
	type->addItem("Abilities");
	type->addItem("Upgrades");
	type->addItem("Buffs");
	search->setPlaceholderText("Search Icons");

	//connect(type, &QComboBox::currentTextChanged, filter, &QSortFilterProxyModel::setFilterFixedString);
	connect(search, &QLineEdit::textEdited, filter, &QSortFilterProxyModel::setFilterFixedString);

	connect(view->selectionModel(), &QItemSelectionModel::selectionChanged, [&]() {
		if (!view->currentIndex().isValid()) {
			finalPath->clear();
			return;
		}
		
		finalPath->setText(filter->data(view->currentIndex(), Qt::EditRole).toString());
	});
}

QString IconView::currentIconPath() {
	return finalPath->text();
}

void IconView::setCurrentIconPath(QString path) {
	finalPath->setText(path);
}