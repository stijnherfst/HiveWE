#pragma once

#include <QDialog>
#include <QShortcut>

#include <vector>

import QRibbon;

class Brush;

/// Palette is the base for all other palette kinds and facilitates things like brush switching and shortcut management
class Palette : public QDialog {
	Q_OBJECT

public:
	/// `active_brush` is the slot that world editing input is routed through. The palette claims it while it
	/// is the active window and lets go of it again when it closes, so the slot has to outlive the palette.
	Palette(QWidget* parent, Brush*& active_brush);
	~Palette();

	std::vector<QShortcut*> shortcuts;

	//void addShortcut(const QKeySequence sequence, const std::vector<QWidget*>& attach_to);

protected:
	/// Makes `brush` the one that world editing input goes to
	void claim_brush(Brush* brush);

	/// Stops world editing input from going to `brush`. Another palette may have claimed the slot in the
	/// meantime, in which case it is left alone.
	void release_brush(Brush* brush);

private:
	Brush*& active_brush;

signals:
	void ribbon_tab_requested(QRibbonTab* tab, QString name);

public slots:
	virtual void deactivate(QRibbonTab* tab) = 0;
};
