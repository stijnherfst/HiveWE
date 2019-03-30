#include "stdafx.h"
#include <Qsci/qsciapis.h>

void Styling::setKeywords(QStringList list) {
	keywords_ = std::move(list);
}

void Styling::setOperators(QStringList list) {
	operators_ = std::move(list);
}

void Styling::setNatives(QStringList list) {
	natives_ = std::move(list);
}

void Styling::setFunctions(QStringList list) {
	functions_ = std::move(list);
}

void Styling::setConstants(QStringList list) {
	constants_ = std::move(list);
}

void Styling::setTypes(QStringList list) {
	types_ = std::move(list);
}

Styling::Styling(QWidget* parent) : QsciLexerCustom(parent) {
	setDefaultFont(QFont("Consolas", 10));

	setColor(QColor(181, 206, 168), JASS_NUMBER); // numbers
	setColor(QColor(56, 156, 214), JASS_KEYWORD); // keywords
	setColor(QColor(214, 157, 133), JASS_STRING); // string
	setColor(QColor(87, 166, 74), JASS_COMMENT); // comment

	std::vector<std::string> operators = { "+", "-", "/", "*", ",", "=", ":", "(", ")", ">=", "<=", "!=", "[", "]", "<", ">", "&" };

	// TODO@Daniel:
	// Types should be in their own list
	keywords_ = QStringList({ "class", "return", "if", "else", "while", "for", "in", "break", "new", "null", "package", "endpackage",
		"function", "returns", "public", "private", "protected", "import", "initlater", "native", "nativetype", "extends", "interface",
		"implements", "module", "use", "abstract", "static", "thistype", "override", "immutable", "it", "array", "and", "or", "not",
		"this", "construct", "ondestroy", "destroy", "type", "constant", "endfunction", "nothing", "init", "castTo", "tuple", "div",
		"mod", "let", "from", "to", "downto", "step", "endpackage", "skip", "true", "false", "var", "instanceof", "super", "enum",
		"switch", "case", "default", "typeId", "begin", "end", "compiletime", "library", "endlibrary", "scope", "endscope", "requires",
		"uses", "needs", "struct", "endstruct", "then", "endif", "loop", "exitwhen", "endloop", "method", "takes", "endmethod", "set",
		"call", "globals", "endglobals", "initializer", "elseif", "vararg", "local" });
}

const char* Styling::language() const {
	return "VJass";
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

		if (i.startsWith("//")) { // Comments
			setStyling(i.length(), 4); 
		} else if (i == "/*") { // Multiline Comments
			multiline = true;
			setStyling(i.length(), 4); 
		} else if (blocks.contains(i)) { // Keywords
			setStyling(i.length(), 2);
		} else if (i.contains(QRegExp(R"(^[0-9]+$)"))) { // Numbers
			setStyling(i.length(), 1);
		} else if (i.contains(QRegExp(R"(^\".*\"$)"))) { // Strings
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
	setAutoIndent(true);
	setTabWidth(4);

	setAutoCompletionSource(QsciScintilla::AutoCompletionSource::AcsAPIs);
	setAutoCompletionUseSingle(QsciScintilla::AcusExplicit);
	setAutoCompletionReplaceWord(false);
	setAutoCompletionThreshold(1);

	SendScintilla(SCI_STYLESETBACK, STYLE_BRACELIGHT, qRgb(30, 75, 125));
	SendScintilla(SCI_STYLESETBACK, STYLE_BRACEBAD, qRgb(125, 60, 25));
	setBraceMatching(QsciScintilla::BraceMatch::SloppyBraceMatch);

	// Column selections
	SendScintilla(SCI_SETMULTIPLESELECTION, true);
	SendScintilla(SCI_SETVIRTUALSPACEOPTIONS, SCVS_RECTANGULARSELECTION | SCVS_NOWRAPLINESTART);
	SendScintilla(SCI_SETADDITIONALSELECTIONTYPING, true);
	SendScintilla(SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH);

	setCallTipsBackgroundColor(palette().color(QPalette::ColorRole::Base));
	setCallTipsForegroundColor(palette().color(QPalette::ColorRole::Text).darker());
	setCallTipsHighlightColor(palette().color(QPalette::ColorRole::Text));


	auto apis = new QsciAPIs(lexer);
	lexer->setAPIs(apis);

	// Very rough and temporary parsing of the script files
	std::stringstream file;
	file << hierarchy.open_file("Scripts/common.j").buffer.data();
	file << hierarchy.open_file("Scripts/blizzard.j").buffer.data();
	file << hierarchy.open_file("Scripts/cheats.j").buffer.data();

	std::string line;
	while (std::getline(file, line)) {
		QString linee = QString::fromStdString(line).simplified();
		
		if (linee.startsWith("type")) {
			apis->add(linee.mid(5, linee.indexOf(' ', 5) + 1 - 5));
		}
		if (linee.startsWith("native")) {
			apis->add(linee.mid(7, linee.indexOf(' ', 7) + 1 - 7));
		}

		if (linee.startsWith("function")) {
			apis->add(linee.mid(9, linee.indexOf(' ', 9) + 1 - 9));
			auto splito = linee.splitRef(',');

		}

		if (linee.startsWith("constant")) {
			int index = linee.indexOf(' ', 9) + 1;
			apis->add(linee.mid(index, linee.indexOf(' ', index) + 1 - index));
		}
	}
	apis->prepare();

	connect(this, &QsciScintilla::textChanged, this, &JassEditor::calculate_margin_width);
}

void JassEditor::calculate_margin_width() {
	const int new_width = std::log10(lines()) + 2;
	if (new_width != max_line_number_width) {
		max_line_number_width = new_width;
		setMarginWidth(0, QString('9').repeated(new_width));
	}
}