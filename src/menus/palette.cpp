#include "palette.h"

Palette::Palette(QWidget* parent) : QDialog(parent) {
}

Palette::~Palette()
{
}

//void Palette::addShortcut(const QKeySequence sequence, const std::vector<QWidget*>& attach_to) {
//	for (auto&& i : attach_to) {
//		shortcuts.push_back(new QShortcut(sequence, i));
//	}
//}