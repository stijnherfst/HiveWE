#include "stdafx.h"

SettingsEditor::SettingsEditor(QWidget* parent) : QDialog(parent) {
	ui.setupUi(this);
	QSettings settings;
	ui.testArgs->setText(settings.value("testArgs").toString());

	connect(ui.buttonBox, &QDialogButtonBox::accepted, [&]() {
		save();
		emit accept();
		close();
	});

	connect(ui.buttonBox, &QDialogButtonBox::rejected, [&]() {
		emit reject();
		close();
	});
	show();
}

void SettingsEditor::save() const {
	QSettings settings;
	settings.setValue("testArgs", ui.testArgs->text());
}