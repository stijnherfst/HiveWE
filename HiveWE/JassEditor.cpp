#include "stdafx.h"
#include <Qsci/qsciapis.h>

#include "JassTokenizer.h"

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

	// TODO@Daniel:
	// Set from external configuration
	// I suggest YAML since it is very intuitive to use, though that may be just a personal preference
	setColor(QColor(181, 206, 168), JASS_NUMBER); // numbers
	setColor(QColor(56, 156, 214), JASS_KEYWORD); // keywords
	setColor(QColor(214, 157, 133), JASS_STRING); // string
	setColor(QColor(214, 157, 133), JASS_RAWCODE); // rawcode
	setColor(QColor(87, 166, 74), JASS_COMMENT); // comment
	setColor(QColor(155, 155, 155), JASS_PREPROCESSOR_COMMENT); // preprocessor comment
	setColor(QColor(189, 99, 197), JASS_NATIVE); // native
	setColor(QColor(200, 100, 100), JASS_FUNCTION); // function
	setColor(QColor(184, 215, 163), JASS_CONSTANT); // constant
	setColor(QColor(78, 201, 176), JASS_TYPE); // type

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
	case JASS_DEFAULT:
		return "Default";
	case JASS_COMMENT:
		return "Comment";
	case JASS_PREPROCESSOR_COMMENT:
		return "Preprocessor comment";
	case JASS_STRING:
		return "String literal";
	case JASS_NUMBER:
		return "Numbers";
	case JASS_OPERATOR:
		return "Operators";
	case JASS_NATIVE:
		return "Natives";
	case JASS_FUNCTION:
		return "Functions";
	case JASS_CONSTANT:
		return "Constant globals";
	case JASS_TYPE:
		return "Types and structs";
	}
	return "Unknown";
}

void Styling::styleText(int start, int end) {
	startStyling(start);

	QString text = editor()->text().mid(start, end - start);

	JassTokenizer tokenizer(text);

	int idx = 0;
	while (tokenizer[idx].type() != TOKEN_EOF)
	{
		int start = tokenizer[idx].start();
		int next_idx = idx + 1;

		JassStyle style = JASS_DEFAULT;

		// TODO@Daniel:
		// The token type checks should probably be moved to a member function in Token

		switch (tokenizer[idx].type())
		{
		case TOKEN_COMMENT_START:
			style = JASS_COMMENT;
			while (tokenizer[next_idx].type() != TOKEN_NEWLINE && tokenizer[next_idx].type() != TOKEN_EOF)
			{
				next_idx++;
			}
			break;
		case TOKEN_PREPROCESSOR_COMMENT_START:
			style = JASS_PREPROCESSOR_COMMENT;
			while (tokenizer[next_idx].type() != TOKEN_NEWLINE && tokenizer[next_idx].type() != TOKEN_EOF)
			{
				next_idx++;
			}
			break;
		case TOKEN_COMMENT_BLOCK_START:
			style = JASS_COMMENT;
			while (tokenizer[next_idx].type() != TOKEN_COMMENT_BLOCK_END && tokenizer[next_idx].type() != TOKEN_EOF)
			{
				next_idx++;
			}
			break;
		case TOKEN_DOUBLE_QUOTE:
			style = JASS_STRING;
			while (tokenizer[next_idx].type() != TOKEN_DOUBLE_QUOTE && tokenizer[next_idx].type() != TOKEN_EOF)
			{
				next_idx++;
			}
			next_idx++;
			break;
		case TOKEN_SINGLE_QUOTE:
			style = JASS_RAWCODE;
			while (tokenizer[next_idx].type() != TOKEN_SINGLE_QUOTE && tokenizer[next_idx].type() != TOKEN_EOF)
			{
				next_idx++;
			}
			next_idx++;
			break;
		case TOKEN_NUMBER:
			style = JASS_NUMBER;
			break;
		case TOKEN_IDENTIFIER:
			JassToken const &token = tokenizer[idx];
			if (natives_.contains(token.value()))
			{
				style = JASS_NATIVE;
			}
			else if (functions_.contains(token.value()))
			{
				style = JASS_FUNCTION;
			}
			else if (constants_.contains(token.value()))
			{
				style = JASS_CONSTANT;
			}
			else if (types_.contains(token.value()))
			{
				style = JASS_TYPE;
			}
			else if (keywords_.contains(token.value()))
			{
				style = JASS_KEYWORD;
			}
			break;
		}

		int length = tokenizer[next_idx].start() - start;

		setStyling(length, style);

		idx = next_idx;
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