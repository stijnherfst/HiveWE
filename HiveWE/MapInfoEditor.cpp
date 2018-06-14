#include "stdafx.h"

MapInfoEditor::MapInfoEditor(QWidget *parent) : QDialog(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	ui.name->setText(QString::fromStdString(map.info.name));
	ui.suggestedPlayers->setText(QString::fromStdString(map.info.suggested_players));
	ui.description->setPlainText(QString::fromStdString(map.info.description));
	ui.author->setText(QString::fromStdString(map.info.author));

	ui.mapVersion->setText(QString::number(map.info.map_version));
	ui.editorVersion->setText(QString::number(map.info.editor_version));

	ui.loadingScreenTitle->setText(QString::fromStdString(map.info.loading_screen_title));
	ui.loadingScreenSubtitle->setText(QString::fromStdString(map.info.loading_screen_subtitle));
	ui.loadingScreenText->setPlainText(QString::fromStdString(map.info.loading_screen_text));

	show();
}