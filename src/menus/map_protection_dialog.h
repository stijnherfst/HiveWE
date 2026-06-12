#pragma once

#include <QDialog>

class QCheckBox;

class MapProtectionDialog : public QDialog {
	Q_OBJECT

  public:
	explicit MapProtectionDialog(QWidget* parent = nullptr);

  private:
	QCheckBox* removeWorldEditorFiles = nullptr;
	QCheckBox* removeMetadata = nullptr;
	QCheckBox* obfuscateScript = nullptr;
	QCheckBox* junkHeaderOffset = nullptr;
	QCheckBox* encryptImports = nullptr;
	QCheckBox* askEachExport = nullptr;

	void save() const;
};
