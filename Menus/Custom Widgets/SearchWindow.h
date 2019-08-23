#include <string>

#include <QFrame>
#include <QPushButton>
#include <QLineEdit>

class SearchWindow : public QFrame {
	Q_OBJECT

	QLineEdit* edit = new QLineEdit;
	QPushButton* case_sensitive = new QPushButton;
	QPushButton* match_whole_word = new QPushButton;
	QPushButton* regular_expression = new QPushButton;

public:
	SearchWindow(QWidget* parent = nullptr);

signals:
	void text_changed(QString text);
	void previous();
	void next();
};