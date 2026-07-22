#include "map_protection_dialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>

MapProtectionDialog::MapProtectionDialog(QWidget* parent) : QDialog(parent) {
	setWindowModality(Qt::ApplicationModal);
	setWindowTitle("Map Protection");
	resize(500, 400);

	auto* info = new QLabel(
		"Map protection makes it harder for people to open your map, but it can never be made impossible.\n"
		"Also consider to release your map without protection so others can fix bugs once you are gone!"
	);
	info->setWordWrap(true);

	auto* effectiveGroup = new QGroupBox("Effective");
	effectiveGroup->setToolTip("These actually deny information and cannot be undone.");
	auto* effectiveLayout = new QVBoxLayout(effectiveGroup);

	removeWorldEditorFiles = new QCheckBox("Remove World Editor only files");
	removeWorldEditorFiles->setToolTip("Removes editor only files that the game does not need.");

	removeMetadata = new QCheckBox("Omit MPQ metadata ((listfile) / (attributes))");
	removeMetadata->setToolTip("Prevents trivial enumeration of the archive contents.");

	obfuscateScript = new QCheckBox("Minify map script (war3map.j / war3map.lua)");
	obfuscateScript->setToolTip("Removes comment-only lines and indentation. Safe minification; does not rename identifiers.");

	effectiveLayout->addWidget(removeWorldEditorFiles);
	effectiveLayout->addWidget(removeMetadata);
	effectiveLayout->addWidget(obfuscateScript);

	auto* ineffectiveGroup = new QGroupBox("Limited effectiveness");
	ineffectiveGroup->setToolTip("Only stops outdated or incomplete tools. Modern extractors (StormLib) are not fooled.");
	auto* ineffectiveLayout = new QVBoxLayout(ineffectiveGroup);

	junkHeaderOffset = new QCheckBox("Offset MPQ header (junk prefix)");
	junkHeaderOffset->setToolTip(
		"Prepends junk so the archive header no longer sits at the start of the file. The game still opens it, but tools that assume a zero offset fail."
	);

	encryptImports = new QCheckBox("Encrypt imported files");
	encryptImports->setToolTip(
		"Encrypts your files so they can't be extracted without knowing the filename. Except that V1 MPQ encryption has been cracked so this will stop very few people."
	);

	ineffectiveLayout->addWidget(junkHeaderOffset);
	ineffectiveLayout->addWidget(encryptImports);

	askEachExport = new QCheckBox("Always ask before export");
	askEachExport->setToolTip("Show this dialog each time the map is exported. When off, the saved options are used silently.");

	auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);

	auto* bottomLayout = new QHBoxLayout();
	bottomLayout->addWidget(askEachExport);
	bottomLayout->addStretch();
	bottomLayout->addWidget(buttonBox);

	auto* layout = new QVBoxLayout(this);
	layout->addWidget(info);
	layout->addWidget(effectiveGroup);
	layout->addWidget(ineffectiveGroup);
	layout->addStretch();
	layout->addLayout(bottomLayout);

	const QSettings settings;
	removeWorldEditorFiles->setChecked(settings.value("protection/removeWorldEditorFiles", true).toBool());
	removeMetadata->setChecked(settings.value("protection/removeMetadata", true).toBool());
	obfuscateScript->setChecked(settings.value("protection/obfuscateScript", false).toBool());
	junkHeaderOffset->setChecked(settings.value("protection/junkHeaderOffset", false).toBool());
	encryptImports->setChecked(settings.value("protection/encryptImports", false).toBool());
	askEachExport->setChecked(settings.value("protection/askEachExport", true).toBool());

	connect(buttonBox, &QDialogButtonBox::accepted, [&]() {
		save();
		emit accept();
		close();
	});

	connect(buttonBox, &QDialogButtonBox::rejected, [&]() {
		emit reject();
		close();
	});
}

void MapProtectionDialog::save() const {
	QSettings settings;
	settings.setValue("protection/removeWorldEditorFiles", removeWorldEditorFiles->isChecked());
	settings.setValue("protection/removeMetadata", removeMetadata->isChecked());
	settings.setValue("protection/obfuscateScript", obfuscateScript->isChecked());
	settings.setValue("protection/junkHeaderOffset", junkHeaderOffset->isChecked());
	settings.setValue("protection/encryptImports", encryptImports->isChecked());
	settings.setValue("protection/askEachExport", askEachExport->isChecked());
}
