#include "SettingsEditor.h"

#include <QSettings>
#include <QFile>

SettingsEditor::SettingsEditor(QWidget* parent) : QDialog(parent) {
	ui.setupUi(this);
	QSettings settings;
	ui.testArgs->setText(settings.value("testArgs").toString());
	ui.theme->setCurrentIndex((settings.value("theme").toString())=="Dark");
	ui.comments->setChecked(settings.value("comments").toString() != "False");

	connect(ui.buttonBox, &QDialogButtonBox::accepted, [&]() {
		save();
		QSettings settings;
		QFile file("Data/Themes/" + settings.value("theme").toString() + ".qss");
		file.open(QFile::ReadOnly);
		QString StyleSheet = QLatin1String(file.readAll());

		qApp->setStyleSheet(StyleSheet);
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
	settings.setValue("theme", ui.theme->currentText());
	settings.setValue("comments", ui.comments->isChecked() ? "True" : "False");
}