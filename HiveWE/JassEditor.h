#pragma once

class Styling : public QsciLexerCustom {
	Q_OBJECT

	QStringList blocks;

public:
	Styling(QWidget* parent);
	const char* language() const override;
	QString description(int style) const override;
	void styleText(int start, int end) override;
};

class JassEditor : public QsciScintilla {
	Q_OBJECT

public:
	JassEditor(QWidget* parent = nullptr);

	Styling* lexer = new Styling(this);
	int max_line_number_width;

	void calculate_margin_width();
};