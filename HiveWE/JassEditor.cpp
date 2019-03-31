#include "stdafx.h"
#include <Qsci/qsciapis.h>

int Styling::styleAt(int idx) const
{
	if (idx > 0)
	{
		return editor()->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, idx);
	}
	return JASS_DEFAULT;
}

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
	setColor(QColor(255, 204, 0), JASS_ESCAPE_SEQUENCE); // character escape sequences
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

	// TODO@Daniel:
	// Eat up unfinished string/rawcode/comment block 

	JassTokenizer tokenizer(editor()->text(), start);

	do {
		JassStyle style = JASS_DEFAULT;

		JassToken token = tokenizer.next();

		switch (token.type()) {
		case TOKEN_COMMENT_LINE:
			style = JASS_COMMENT;
			break;
		case TOKEN_PREPROCESSOR_COMMENT:
			style = JASS_PREPROCESSOR_COMMENT;
			break;
		case TOKEN_COMMENT_BLOCK:
			style = JASS_COMMENT;
			break;
		case TOKEN_STRING:
			for (JassToken const& sequence : token.nested_tokens())
			{
				setStyling(sequence.start() - start, JASS_STRING);
				setStyling(sequence.length(), JASS_ESCAPE_SEQUENCE);

				start = sequence.stop();
			}
			style = JASS_STRING;
			break;
		case TOKEN_RAWCODE:
			style = JASS_RAWCODE;
			break;
		case TOKEN_NUMBER:
			style = JASS_NUMBER;
			break;
		case TOKEN_IDENTIFIER:
			if (natives_.contains(token.value())) {
				style = JASS_NATIVE;
			}
			else if (functions_.contains(token.value())) {
				style = JASS_FUNCTION;
			}
			else if (constants_.contains(token.value())) {
				style = JASS_CONSTANT;
			}
			else if (types_.contains(token.value())) {
				style = JASS_TYPE;
			}
			else if (keywords_.contains(token.value())) {
				style = JASS_KEYWORD;
			}
			break;
		}

		int length = token.stop() - start;

		setStyling(length, style);

		start = token.stop();
	}
	while (start < tokenizer.text_size());
}

bool Styling::caseSensitive() const {
	return false;
}

JassEditor::JassEditor(QWidget* parent) : QsciScintilla(parent), lexer(this) {
	setLexer(&lexer);
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

	QStringList types;
	QStringList natives;
	QStringList functions;
	QStringList constants;

	// Primitive types
	types.append("nothing");
	types.append("boolean");
	types.append("code");
	types.append("integer");
	types.append("real");
	types.append("string");
	types.append("handle");

	//auto apis = new QsciAPIs(lexer);
	//lexer->setAPIs(apis);

	// Very rough and temporary parsing of the script files
	std::stringstream file;
	file << hierarchy.open_file("Scripts/common.j").buffer.data();
	file << hierarchy.open_file("Scripts/blizzard.j").buffer.data();
	file << hierarchy.open_file("Scripts/cheats.j").buffer.data();

	std::string line;
	while (std::getline(file, line)) {
		QString linee = QString::fromStdString(line).simplified();

		if (linee.startsWith("type")) {
			QString type = linee.mid(5, linee.indexOf(' ', 5) + 1 - 5).trimmed();
			//apis->add(type);
			types.append(type);
		}
		if (linee.startsWith("native")) {
			QString native = linee.mid(7, linee.indexOf(' ', 7) + 1 - 7).trimmed();
			//apis->add(native);
			natives.append(native);
		}

		if (linee.startsWith("function")) {
			QString function = linee.mid(9, linee.indexOf(' ', 9) + 1 - 9).trimmed();
			//apis->add(function);
			functions.append(function);

			auto splito = linee.splitRef(',');

		}

		if (linee.startsWith("constant")) {
			int index = linee.indexOf(' ', 9) + 1;

			QString constant = linee.mid(index, linee.indexOf(' ', index) + 1 - index).trimmed();
			//apis->add(constant);
			constants.append(constant);
		}
	}
	//apis->prepare();

	lexer.setTypes(types);
	lexer.setNatives(natives);
	lexer.setFunctions(functions);
	lexer.setConstants(constants);

	connect(this, &QsciScintilla::textChanged, this, &JassEditor::calculate_margin_width);
}

void JassEditor::calculate_margin_width() {
	const int new_width = std::log10(lines()) + 2;
	if (new_width != max_line_number_width) {
		max_line_number_width = new_width;
		setMarginWidth(0, QString('9').repeated(new_width));
	}
}