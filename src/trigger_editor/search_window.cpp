#include "search_window.h"

#include <QLayout>
#include <QFrame>

SearchWindow::SearchWindow(QWidget* parent) : QFrame(parent) {
	setAttribute(Qt::WA_DeleteOnClose);

	setMinimumSize(182, 53);
	setAutoFillBackground(true);
	setFrameStyle(QFrame::NoFrame);

	// Top row
	edit->setPlaceholderText("Find text");

	QPushButton* close_button = new QPushButton(this);
	close_button->setText("X");
	close_button->setMaximumSize(23, 23);

	QHBoxLayout* top_row = new QHBoxLayout;
	top_row->addWidget(edit);
	top_row->addWidget(close_button);

	// Bottom row
	case_sensitive->setCheckable(true);
	case_sensitive->setMaximumSize(23, 23);
	match_whole_word->setCheckable(true);
	match_whole_word->setMaximumSize(23, 23);
	regular_expression->setCheckable(true);
	regular_expression->setMaximumSize(23, 23);

	QHBoxLayout* bottom_row = new QHBoxLayout;
	bottom_row->addWidget(case_sensitive);
	bottom_row->addWidget(match_whole_word);
	bottom_row->addWidget(regular_expression);
	bottom_row->addStretch();

	QVBoxLayout* layout = new QVBoxLayout;
	layout->addLayout(top_row);
	layout->addLayout(bottom_row);
	layout->setSpacing(3);

	setLayout(layout);
	show();

	connect(edit, &QLineEdit::textChanged, this, &SearchWindow::text_changed);
	connect(close_button, &QPushButton::clicked, this, &QWidget::close);
}