#include "stdafx.h"

Styling::Styling(QWidget* parent) : QsciLexerCustom(parent) {
	setDefaultColor(QColor("#ff000000"));
	setDefaultPaper(QColor("#ffffffff"));
	setDefaultFont(QFont("Consolas", 10));

	setColor(QColor("#ff000000"), 0);
	setColor(QColor("#ff7f0000"), 1);
	setColor(QColor("#ff0000bf"), 2);
	setColor(QColor("#ff007f00"), 3);
	setColor(QColor(0, 200, 0), 4);

	setPaper(QColor("#ffffffff"), 0);
	setPaper(QColor("#ffffffff"), 1);
	setPaper(QColor("#ffffffff"), 2);
	setPaper(QColor("#ffffffff"), 3);
	setPaper(QColor("#ffffffff"), 4);

	setFont(QFont("Consolas", 10, QFont::Normal), 0);
	setFont(QFont("Consolas", 10, QFont::Normal), 1);
	setFont(QFont("Consolas", 10, QFont::Normal), 2);
	setFont(QFont("Consolas", 10, QFont::Normal), 3);
	setFont(QFont("Consolas", 10, QFont::Normal), 4);

	std::vector<std::string> operators = { "+", "-", "/", "*", ",", "=", ":", "(", ")", ">=", "<=", "!=", "[", "]", "<", ">", "&" };
	blocks = QStringList({ "native", "constant", "exitwhen", "takes", "returns", "private", "public", "keyword", "static", 
		"delegate", "initializer", "uses", "needs", "requires", "defaults", "interface", "endinterface", "globals", "endglobals", 
		"function", "endfunction", "scope", "endscope", "library", "library_once", "endlibrary", "struct", "endstruct", "method", 
		"endmethod", "loop", "endloop", "if", "then", "else", "elseif", "endif", "operator", "readonly", "stub", "module", "endmodule", 
		"implement", "optional" });
}

const char* Styling::language() const {
	return "Ayylmao";
}

QString Styling::description(int style) const {
	switch (style) {
		case 0:
			return "Style0";
		case 1:
			return "Style1";		
		case 2:
			return "Style2";
		case 3:
			return "Style3";
		case 4:
			return "Style4";
	}
	return "Idunnoman";
}

void Styling::styleText(int start, int end) {
	startStyling(start);

	QString text = editor()->text().mid(start, end - start);



	QRegExp expression(R"(([*]\/|\/[*]|\s+|\w+|\W))");
	QStringList tokens;
	int pos = 0;
	while ((pos = expression.indexIn(text, pos)) != -1) {
		tokens << expression.cap(1);
		pos += expression.matchedLength();
	}

	bool linecomment = false;
	bool multiline = false;
	for (auto&& i : tokens) {
		/*if (linecomment) {
			linecomment = (i != "\n");
			setStyling(i.length(), 4);
		}
		if (i == "//") {
			linecomment = true;
			setStyling(i.length(), 4);*/
		if (blocks.contains(i)) {
			setStyling(i.length(), 2);
		} else if (i.contains(QRegExp("[0-9]+(.[0-9]*)?"))) {
			setStyling(i.length(), 1);
		} else {
			setStyling(i.length(), 0);
		}
	}

}



JassEditor::JassEditor(QWidget *parent) : QsciScintilla(parent) {
	setMargins(1);
	setMarginType(0, QsciScintilla::MarginType::NumberMargin);
	setMarginWidth(0, 100);
	setMarginLineNumbers(0, true);

	setLexer(lexer);

	connect(this, &QsciScintilla::textChanged, this, &JassEditor::calculate_margin_width);
}

void JassEditor::calculate_margin_width() {
	const int new_width = std::log10(lines()) + 2;
	if (new_width != max_line_number_width) {
		max_line_number_width = new_width;
		setMarginWidth(0, QString('9').repeated(new_width));
	}
}