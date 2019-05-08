#include "stdafx.h"

ImportManagerEdit::ImportManagerEdit(QWidget* parent) : QDialog(parent) {
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);

	connect(ui.customPath, &QCheckBox::clicked, [this](bool checked) {
		ui.fileFullPath->setEnabled(checked);
		if (!checked) {
			std::string filename = fs::path(ui.fileFullPath->text().toStdString()).filename().string();
			ui.fileFullPath->setText("war3mapImported\\" + QString::fromStdString(filename));
		}
	});

	connect(ui.buttonBox, &QDialogButtonBox::accepted, [this]() {
		if (ui.fileFullPath->text().startsWith("war3mapImported\\")) {
			emit accepted(false, ui.fileFullPath->text());
		} else {
			emit accepted(ui.customPath->isChecked(), ui.fileFullPath->text());
		}
	});

	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);
}