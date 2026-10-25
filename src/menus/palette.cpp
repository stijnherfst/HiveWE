#include "palette.h"

#include <QKeyEvent>

Palette::Palette(QWidget* parent, Brush*& active_brush) : QDialog(parent), active_brush(active_brush) {
}

Palette::~Palette()
{
}

void Palette::claim_brush(Brush* brush) {
	active_brush = brush;
}

void Palette::release_brush(Brush* brush) {
	if (active_brush == brush) {
		active_brush = nullptr;
	}
}

void Palette::keyPressEvent(QKeyEvent* event) {
	// QDialog handles Escape with reject() which filters QEvent::Close so need to call it manually
	if (event->key() == Qt::Key_Escape && event->modifiers() == Qt::NoModifier) {
		close();
		return;
	}
	QDialog::keyPressEvent(event);
}

//void Palette::addShortcut(const QKeySequence sequence, const std::vector<QWidget*>& attach_to) {
//	for (auto&& i : attach_to) {
//		shortcuts.push_back(new QShortcut(sequence, i));
//	}
//}
