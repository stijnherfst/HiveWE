#pragma once

#include <Qsci/qsciapis.h>

#include <QSet>
#include <QString>

#include "JassTokenizer.h"

class Styling : public QsciLexerCustom {
private:
	Q_OBJECT

		// TODO@Daniel:
		// Rearrange
		QSet<QString> keywords_;
	QSet<QString> natives_;
	QSet<QString> functions_;
	QSet<QString> constants_;
	QSet<QString> operators_;
	QSet<QString> types_;

	enum JassStyle {
		// NOTE@Daniel:
		// Style IDs may change
		JASS_DEFAULT = QsciScintilla::STYLE_DEFAULT,
		JASS_COMMENT = QsciScintilla::STYLE_LASTPREDEFINED,
		JASS_PREPROCESSOR_COMMENT,
		JASS_STRING,
		JASS_ESCAPE_SEQUENCE,
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

	void setKeywords(QSet<QString> list);
	void setOperators(QSet<QString> list);
	void setNatives(QSet<QString> list);
	void setFunctions(QSet<QString> list);
	void setConstants(QSet<QString> list);
	void setTypes(QSet<QString> list);

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
	QsciAPIs api;

public:
	JassEditor(QWidget* parent = nullptr);

	int max_line_number_width;

	void calculate_margin_width();
};