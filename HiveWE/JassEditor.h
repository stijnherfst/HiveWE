#pragma once

#include "JassTokenizer.h"

class Styling : public QsciLexerCustom {
private:
	Q_OBJECT

	// TODO@Daniel:
	// Rearrange
	QStringList keywords_;
	QStringList natives_;
	QStringList functions_;
	QStringList constants_;
	QStringList operators_;
	QStringList types_;

	enum JassStyle {
		// NOTE@Daniel:
		// Style IDs may change
		JASS_DEFAULT = QsciScintilla::STYLE_DEFAULT,
		JASS_COMMENT = QsciScintilla::STYLE_LASTPREDEFINED,
		JASS_PREPROCESSOR_COMMENT,
		JASS_STRING,
		JASS_RAWCODE,
		JASS_KEYWORD,
		JASS_NUMBER,

		// NOTE@Daniel:
		// Having a separate style is really useful, in my opinion
		// However, I don't know if there should be a distinction between different operators
		JASS_OPERATOR,

		// TODO@Daniel:
		// Make distinction between user declared natives/functions and those in common.j/common.ai/blizzard.j
		JASS_NATIVE,
		JASS_FUNCTION,
		JASS_CONSTANT,
		JASS_TYPE
	};

public:
	int styleAt(int idx) const;

	int styleToken(JassToken const& token, int start);

	void setKeywords(QStringList list);
	void setOperators(QStringList list);
	void setNatives(QStringList list);
	void setFunctions(QStringList list);
	void setConstants(QStringList list);
	void setTypes(QStringList list);

	Styling(QWidget* parent);
	const char* language() const override;
	QString description(int style) const override;
	void styleText(int start, int end) override;
	bool caseSensitive() const override;
};

class JassEditor : public QsciScintilla {
private:
	Q_OBJECT

	Styling lexer;
public:
	JassEditor(QWidget* parent = nullptr);

	int max_line_number_width;

	void calculate_margin_width();
};