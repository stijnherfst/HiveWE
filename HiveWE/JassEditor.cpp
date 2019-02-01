#include "stdafx.h"
#include <Qsci/qsciapis.h>


Styling::Styling(QWidget* parent) : QsciLexerCustom(parent) {
	setDefaultFont(QFont("Consolas", 10));
	//setDefaultColor(QColor("#ff000000"));
	//setDefaultPaper(QColor("#ffffffff"));

	//setColor(QColor("#ff000000"), 0);
	setColor(QColor(181, 206, 168), 1); // numbers
	setColor(QColor(56, 156, 214), 2); // keywords
	setColor(QColor(214, 157, 133), 3); // string
	setColor(QColor(87, 166, 74), 4); // comment

	//setPaper(QColor("#ffffffff"), 0);
	//setPaper(QColor("#ffffffff"), 1);
	//setPaper(QColor("#ffffffff"), 2);
	//setPaper(QColor("#ffffffff"), 3);
	//setPaper(QColor("#ffffffff"), 4);
	
	setFont(QFont("Consolas", 10, QFont::Normal), 0);
	setFont(QFont("Consolas", 10, QFont::Normal), 1);
	setFont(QFont("Consolas", 10, QFont::Normal), 2);
	setFont(QFont("Consolas", 10, QFont::Normal), 3);
	setFont(QFont("Consolas", 10, QFont::Normal), 4);

	std::vector<std::string> operators = { "+", "-", "/", "*", ",", "=", ":", "(", ")", ">=", "<=", "!=", "[", "]", "<", ">", "&" };
	blocks = QStringList({ "class", "return", "if", "else", "while", "for", "in", "break", "new", "null", "package", "endpackage", 
		"function", "returns", "public", "private", "protected", "import", "initlater", "native", "nativetype", "extends", "interface", 
		"implements", "module", "use", "abstract", "static", "thistype", "override", "immutable", "it", "array", "and", "or", "not", 
		"this", "construct", "ondestroy", "destroy", "type", "constant", "endfunction", "nothing", "init", "castTo", "tuple", "div", 
		"mod", "let", "from", "to", "downto", "step", "endpackage", "skip", "true", "false", "var", "instanceof", "super", "enum", 
		"switch", "case", "default", "typeId", "begin", "end", "compiletime", "library", "endlibrary", "scope", "endscope", "requires", 
		"uses", "needs", "struct", "endstruct", "then", "endif", "loop", "exitwhen", "endloop", "method", "takes", "endmethod", "set", 
		"call", "globals", "endglobals", "initializer", "elseif", "vararg", "local" });
}



const char* Styling::language() const {
	return "Vjass";
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
	return "Style0";
}

void Styling::styleText(int start, int end) {
	startStyling(start);

	QString text = editor()->text().mid(start, end - start);

	QRegExp expression(R"((\"\S*\"|\/\/[^\r\n]+|[*]\/|\/[*]|\w+|\W|[*]\/|\/[*]))");
	QStringList tokens;
	int pos = 0;
	while ((pos = expression.indexIn(text, pos)) != -1) {
		tokens << expression.cap(1);
		pos += expression.matchedLength();
	}

	bool multiline = false;
	if (start > 0) {
		int previous_style_nr = editor()->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, start - 1);
		if (previous_style_nr == 4) {
			multiline = true;
		}
	}

	for (auto&& i : tokens) {
		if (multiline) {
			setStyling(i.length(), 4);

			if (i == "*/") {
				multiline = false;
			}
			continue;
		}

		if (i.startsWith("//")) {
			setStyling(i.length(), 4); // Comments
		} else if (i == "/*") {
			multiline = true;
			setStyling(i.length(), 4); // Comments
		} else if (blocks.contains(i)) { // Keywords
			setStyling(i.length(), 2);
		} else if (i.contains(QRegExp(R"(^[0-9]+$)"))) { // Numbers
			setStyling(i.length(), 1);
		} else if (i.contains(QRegExp(R"(^\".*\"$)"))) {
			setStyling(i.length(), 3);
		} else {
			setStyling(i.length(), 0);
		}
	}
}

bool Styling::caseSensitive() const {
	return false;
}

JassEditor::JassEditor(QWidget *parent) : QsciScintilla(parent) {
	setLexer(lexer);
	setCaretForegroundColor(QColor(255, 255, 255));
	setMargins(1);
	setMarginType(0, QsciScintilla::MarginType::NumberMargin);
	setMarginWidth(0, 100);
	setMarginLineNumbers(0, true);

	setIndentationsUseTabs(true);
	setTabIndents(true);
	setIndentationGuides(true);
	setTabWidth(4);

	setAutoCompletionSource(QsciScintilla::AutoCompletionSource::AcsAPIs);
	setAutoCompletionUseSingle(QsciScintilla::AcusExplicit);
	setAutoCompletionReplaceWord(false);
	setAutoCompletionThreshold(1);

	auto tt = new QsciAPIs(lexer);
	lexer->setAPIs(tt);
	auto tot = map->triggers.trigger_data.section("TriggerActions");

	std::stringstream file;
	file << hierarchy.open_file("Scripts/common.j").buffer.data();
	
	std::string hur;
	while (std::getline(file, hur)) {
		if (QString::fromStdString(hur).startsWith("native")) {
			tt->add(QString::fromStdString(hur).mid(7));
		}
	}
	tt->prepare();
	
	SendScintilla(SCI_STYLESETBACK, STYLE_BRACELIGHT, RGB(30, 75, 125));
	SendScintilla(SCI_STYLESETBACK, STYLE_BRACEBAD, RGB(125, 60, 25));

	setBraceMatching(QsciScintilla::BraceMatch::SloppyBraceMatch);
	connect(this, &QsciScintilla::textChanged, this, &JassEditor::calculate_margin_width);
}

void JassEditor::calculate_margin_width() {
	const int new_width = std::log10(lines()) + 2;
	if (new_width != max_line_number_width) {
		max_line_number_width = new_width;
		setMarginWidth(0, QString('9').repeated(new_width));
	}
}