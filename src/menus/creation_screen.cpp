#include "creation_screen.h"

import BinaryWriter;

import MapInfo;

#include <fstream>
#include <iostream>
#include <QDir>
#include <QDialogButtonBox>


CreationScreen::CreationScreen(QWidget* parent, const fs::path path) : QDialog(parent) {
	if (path == "") {
		std::cout << "Map Creation attempted with bad path\n";
		return;
	}
	ui.setupUi(this);

	ui.height->setValidator(new QIntValidator(0, 512, this));
	ui.width->setValidator(new QIntValidator(0, 512, this));

	connect(ui.buttonBox, &QDialogButtonBox::accepted, [&, path]() {
		create(path);
		emit accept();
		close();
	});

	connect(ui.buttonBox, &QDialogButtonBox::rejected, [&]() {
		emit reject();
		close();
	});
	show();
}

void CreationScreen::create(const fs::path path) const {
	int width = ui.width->text().toInt();
	int height = ui.height->text().toInt();
	int mapsize16 = height * width * 16;

	char tileset = 'X';								// maybe implement tileset choice panel?
													// would require changes to the w3e code

	BinaryWriter w3i;
	w3i.write(MapInfo::write_version);
	w3i.write(0);									// Map Version
	w3i.write(MapInfo::write_editor_version);
	w3i.write(MapInfo::write_game_version_major);
	w3i.write(MapInfo::write_game_version_minor);
	w3i.write(MapInfo::write_game_version_patch);
	w3i.write(MapInfo::write_game_version_build);
	w3i.write_c_string("TRIGSTR_001");				// Map name
	w3i.write_c_string("TRIGSTR_002");				// Author
	w3i.write_c_string("TRIGSTR_003");				// Description
	w3i.write_c_string("TRIGSTR_004");				// Suggested players
	for (int i = 0; i < 8; i++)
		w3i.write(0.f);
	for (int i = 0; i < 4; i++)
		w3i.write(0);
	w3i.write(width);
	w3i.write(height);
	w3i.write(0x0004);								// Map flags
	w3i.write(tileset);
	w3i.write(-1);
	for (int i = 0; i < 4; i++)
		w3i.write_c_string("");						// Load screen strings
	w3i.write(0);									// Game data set
	for (int i = 0; i < 4; i++)
		w3i.write_c_string("");						// Prologue Strings
	w3i.write(0);									// Terrain fog
	for (int i = 0; i < 3; i++)
		w3i.write(0.f);
	w3i.write<uint32_t>(0);
	w3i.write(0);									// Weather id
	w3i.write_c_string("");							// Custom sound environment
	w3i.write<uint8_t>(0);							// Custom light tileset
	w3i.write<uint32_t>(-1);						// Water colour
	w3i.write(ui.script->currentIndex());
	w3i.write(3);
	w3i.write(2);

	w3i.write(1);									// One player
	w3i.write(0);
	w3i.write(1);
	w3i.write(1);
	w3i.write(0);
	w3i.write_c_string("TRIGSTR_005");
	w3i.write(0.f);
	w3i.write(0.f);
	w3i.write(0);
	w3i.write(0);
	w3i.write(0);
	w3i.write(0);

	w3i.write(1);
	w3i.write(0);
	w3i.write(-1);
	w3i.write_c_string("TRIGSTR_006");

	for (int i = 0; i < 4; i++)
		w3i.write(0);

	BinaryWriter w3e;
	w3e.write_string("W3E!");
	w3e.write(11);
	w3e.write(tileset);
	w3e.write(1);
	w3e.write(9);
	w3e.write_string("XdrtXdtrXblmXbtlXsqdXrtlXgsbXhdgXwmb");
	w3e.write(2);
	w3e.write_string("CXdiCXsq");
	w3e.write(width + 1);
	w3e.write(height + 1);
	w3e.write(-64.f * width);
	w3e.write(-64.f * height);
	
	auto random_var = [](uint16_t x) -> uint8_t {
		x ^= x >> 7;
		x *= 0x19b3U;
		x ^= x >> 7;
		x *= 0xecb5U;
		x ^= x >> 7;
		return x % 18;
	};

	for (int i = 0; i < (height + 1) * (width + 1); i++) {
		w3e.write<uint16_t>(0x2000);
		w3e.write<uint16_t>(0x2000);
		w3e.write<uint8_t>(0);
		w3e.write<uint8_t>(random_var(i));
		w3e.write<uint8_t>(0xF2);
	}

	BinaryWriter doo;
	doo.write_string("W3do");
	doo.write(8);
	doo.write(11);
	doo.write(0);
	doo.write(0);
	doo.write(0);

	BinaryWriter udoo;
	udoo.write_string("W3do");
	udoo.write(8);
	udoo.write(11);
	udoo.write(1);

	udoo.write_string("sloc");
	udoo.write(0);
	udoo.write(0.f);
	udoo.write(0.f);
	udoo.write(64.f);
	udoo.write(270.f);
	for (int i = 0; i < 3; i++)
		//udoo.write(128.f);	// scale,WC3?
		udoo.write(1.f);		// scale,WE?
	udoo.write_string("sloc");	// skin
	udoo.write<char>(2);
	udoo.write(0);
	udoo.write<char>(0);
	udoo.write<char>(0);
	udoo.write(-1);
	udoo.write(-1);
	udoo.write(-1);
	udoo.write(0);
	udoo.write(0);
	udoo.write(-1.f);
	udoo.write(1);
	for (int i = 0; i < 7; i++)
		udoo.write(0);
	udoo.write(-1);
	udoo.write(-1);
	udoo.write(0);

	BinaryWriter shd;
	for (int i = 0; i < mapsize16; i++)	//4x4 pixels per tile
		shd.write<char>(0);

	BinaryWriter mmp;
	mmp.write(0);
	mmp.write(1);
	mmp.write(2);
	mmp.write(0x80);
	mmp.write(0x80);
	mmp.write<char>(0x03);
	mmp.write<char>(0x03);
	mmp.write<char>(0xFFu);
	mmp.write<char>(0xFFu);

	BinaryWriter w3c;
	w3c.write(0);
	w3c.write(0);

	BinaryWriter w3r;
	w3r.write(5);
	w3r.write(0);

	BinaryWriter wts;
	wts.write<char>(0xEFu);
	wts.write<char>(0xBBu);
	wts.write<char>(0xBFu);
	wts.write_string("STRING 1\r\n{\r\n" + ui.name->text().toStdString()				+ "\r\n}\r\n\r\n");
	wts.write_string("STRING 2\r\n{\r\n" + ui.author->text().toStdString()				+ "\r\n}\r\n\r\n");
	wts.write_string("STRING 3\r\n{\r\n" + ui.description->toPlainText().toStdString()	+ "\r\n}\r\n\r\n");
	wts.write_string("STRING 4\r\n{\r\n" + ui.suggestedPlayers->text().toStdString()	+ "\r\n}\r\n\r\n");
	wts.write_string("STRING 5\r\n{\r\nPlayer 1\r\n}\r\n\r\n");
	wts.write_string("STRING 6\r\n{\r\nForce 1\r\n}\r\n\r\n");

	BinaryWriter wtg;
	wtg.write_string("WTG!");
	wtg.write(0x80000004);
	wtg.write(7);
	wtg.write(1);
	wtg.write(0);
	wtg.write(0);
	wtg.write(0);
	wtg.write(0);
	wtg.write(0);
	wtg.write(0);
	wtg.write(0);
	wtg.write(0);
	wtg.write(0);
	wtg.write(0);
	wtg.write(0);
	wtg.write(0);
	wtg.write(0);
	wtg.write(0);
	wtg.write(0);
	wtg.write(2);
	wtg.write(0);
	wtg.write(1);
	wtg.write(1);
	wtg.write(0);
	wtg.write_c_string(path.filename().string());
	wtg.write(0);
	wtg.write(0);
	wtg.write(0xFFFFFFFF);

	BinaryWriter wct;
	wct.write(0x80000004);
	wct.write(1);
	wct.write_c_string("");
	wct.write(0);

	std::ofstream w3i_file(path / "war3map.w3i", std::ios::binary);
	w3i_file.write(reinterpret_cast<char const*>(w3i.buffer.data()), w3i.buffer.size());
	w3i_file.close();

	std::ofstream w3e_file(path / "war3map.w3e", std::ios::binary);
	w3e_file.write(reinterpret_cast<char const*>(w3e.buffer.data()), w3e.buffer.size());
	w3e_file.close();

	// it makes no sense to create a wpm file here, any world editor will do it on save

	std::ofstream doo_file(path / "war3map.doo", std::ios::binary);
	doo_file.write(reinterpret_cast<char const*>(doo.buffer.data()), doo.buffer.size());
	doo_file.close();

	std::ofstream udoo_file(path / "war3mapUnits.doo", std::ios::binary);
	udoo_file.write(reinterpret_cast<char const*>(udoo.buffer.data()), udoo.buffer.size());
	udoo_file.close();

	std::ofstream shd_file(path / "war3map.shd", std::ios::binary);
	shd_file.write(reinterpret_cast<char const*>(shd.buffer.data()), shd.buffer.size());
	shd_file.close();

	std::ofstream mmp_file(path / "war3map.mmp", std::ios::binary);
	mmp_file.write(reinterpret_cast<char const*>(mmp.buffer.data()), mmp.buffer.size());
	mmp_file.close();

	std::ofstream wct_file(path / "war3map.wct", std::ios::binary);
	wct_file.write(reinterpret_cast<char const*>(wct.buffer.data()), wct.buffer.size());
	wct_file.close();

	std::ofstream w3c_file(path / "war3map.w3c", std::ios::binary);
	w3c_file.write(reinterpret_cast<char const*>(w3c.buffer.data()), w3c.buffer.size());
	w3c_file.close();

	std::ofstream w3r_file(path / "war3map.w3r", std::ios::binary);
	w3r_file.write(reinterpret_cast<char const*>(w3r.buffer.data()), w3r.buffer.size());
	w3r_file.close();

	// The w3s gets deleted when empty

	std::ofstream wtg_file(path / "war3map.wtg", std::ios::binary);
	wtg_file.write(reinterpret_cast<char const*>(wtg.buffer.data()), wtg.buffer.size());
	wtg_file.close();


	// Create all locales' folders for easier localization
	QDir locales(QString::fromStdString(path.string() + "/_Locales"));
	locales.mkpath("./deDE.w3mod");
	locales.mkpath("./enUS.w3mod");
	locales.mkpath("./esES.w3mod");
	locales.mkpath("./esMX.w3mod");
	locales.mkpath("./frFR.w3mod");
	locales.mkpath("./itIT.w3mod");
	locales.mkpath("./koKR.w3mod");
	locales.mkpath("./plPL.w3mod");
	locales.mkpath("./ptBR.w3mod");
	locales.mkpath("./ruRU.w3mod");
	locales.mkpath("./zhCN.w3mod");
	locales.mkpath("./zhTW.w3mod");

	std::ofstream wts_file(path / "war3map.wts", std::ios::binary);
	wts_file.write(reinterpret_cast<char const*>(wts.buffer.data()), wts.buffer.size());
	wts_file.close();

	// Copy the wts file to english locale?
	// wts_file.open(path / "_Locales" / "enUS.w3mod" / "war3map.wts", std::ios::binary);
	// wts_file.write(reinterpret_cast<char const*>(wts.buffer.data()), wts.buffer.size());
	// wts_file.close();
}