#include "stdafx.h"

int Styling::styleAt(int idx) const {
	if (idx > 0) {
		return editor()->SendScintilla(QsciScintilla::SCI_GETSTYLEAT, idx);
	}
	return JASS_DEFAULT;
}

int Styling::styleToken(JassToken const& token, int start) {
	JassStyle style = JASS_DEFAULT;

	switch (token.type()) {
		case TOKEN_COMMENT_LINE:
			style = JASS_COMMENT;
			break;
		case TOKEN_PREPROCESSOR_COMMENT:
			style = JASS_PREPROCESSOR_COMMENT;
			break;
		case TOKEN_COMMENT_BLOCK:
			// TODO@Daniel:
			// Style docstring parameters

			style = JASS_COMMENT;
			break;
		case TOKEN_STRING:
			for (JassToken const& sequence : token.nested_tokens()) {
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
			} else if (functions_.contains(token.value())) {
				style = JASS_FUNCTION;
			} else if (constants_.contains(token.value())) {
				style = JASS_CONSTANT;
			} else if (types_.contains(token.value())) {
				style = JASS_TYPE;
			} else if (keywords_.contains(token.value())) {
				style = JASS_KEYWORD;
			}
			break;
	}

	setStyling(token.stop() - start, style);

	return token.stop();
}

void Styling::setKeywords(QSet<QString> list) {
	keywords_ = std::move(list);
}

void Styling::setOperators(QSet<QString> list) {
	operators_ = std::move(list);
}

void Styling::setNatives(QSet<QString> list) {
	natives_ = std::move(list);
}

void Styling::setFunctions(QSet<QString> list) {
	functions_ = std::move(list);
}

void Styling::setConstants(QSet<QString> list) {
	constants_ = std::move(list);
}

void Styling::setTypes(QSet<QString> list) {
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
	keywords_ = { "class", "return", "if", "else", "while", "for", "in", "break", "new", "null", "package", "endpackage",
		"function", "returns", "public", "private", "protected", "import", "initlater", "native", "nativetype", "extends", "interface",
		"implements", "module", "use", "abstract", "static", "thistype", "override", "immutable", "it", "array", "and", "or", "not",
		"this", "construct", "ondestroy", "destroy", "type", "constant", "endfunction", "nothing", "init", "castTo", "tuple", "div",
		"mod", "let", "from", "to", "downto", "step", "endpackage", "skip", "true", "false", "var", "instanceof", "super", "enum",
		"switch", "case", "default", "typeId", "begin", "end", "compiletime", "library", "endlibrary", "scope", "endscope", "requires",
		"uses", "needs", "struct", "endstruct", "then", "endif", "loop", "exitwhen", "endloop", "method", "takes", "endmethod", "set",
		"call", "globals", "endglobals", "initializer", "elseif", "vararg", "local" };
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
		case JASS_ESCAPE_SEQUENCE:
			return "Character escape sequence";
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

	int starting_style = styleAt(start - 1);

	JassTokenizer tokenizer(editor()->text().mid(start));

	int idx = 0;
	switch (starting_style) {
		case JASS_COMMENT:
			idx = styleToken(tokenizer.parse_comment_block(), idx);
			break;
		case JASS_STRING:
			idx = styleToken(tokenizer.parse_string(), idx);
			break;
		case JASS_RAWCODE:
			idx = styleToken(tokenizer.parse_rawcode(), idx);
			break;
		default:
			break;
	}

	do {
		idx = styleToken(tokenizer.next(), idx);
	} while (idx < tokenizer.text_size() && idx + start < end);
}

bool Styling::caseSensitive() const {
	return true;
}

JassEditor::JassEditor(QWidget * parent) :
	QsciScintilla(parent),
	lexer(this),
	api(&lexer) {

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

	QSet<QString> types;
	QSet<QString> natives;
	QSet<QString> functions;
	QSet<QString> constants;

	// Primitive types
	types.insert("nothing");
	types.insert("boolean");
	types.insert("code");
	types.insert("integer");
	types.insert("real");
	types.insert("string");
	types.insert("handle");

	// NOTE@Daniel:
	// I hadn't noticed that this wasn't a QString before I finished JassTokenizer
	std::vector<uint8_t> common_data = hierarchy.open_file("Scripts/common.j").buffer;
	QString common_script(QByteArray((char const*)common_data.data(), (int)common_data.size()));

	std::vector<uint8_t> blizzard_data = hierarchy.open_file("Scripts/blizzard.j").buffer;
	QString blizzard_script(QByteArray((char const*)blizzard_data.data(), (int)blizzard_data.size()));

	std::vector<uint8_t> cheat_data = hierarchy.open_file("Scripts/cheats.j").buffer;
	QString cheat_script(QByteArray((char const*)cheat_data.data(), (int)cheat_data.size()));

	// TODO@Daniel:
	// This should be in it's own class
	JassTokenizer tokenizer(common_script + '\n' + blizzard_script + '\n' + cheat_script);

	JassToken token = tokenizer.next();
	while (token.type() != TOKEN_EOF) {
		if (token.value() == "function") {
			token = tokenizer.eat_comment_blocks();

			if (token.type() == TOKEN_IDENTIFIER) {
				QString value = token.value();

				token = tokenizer.eat_comment_blocks();
				if (token.value() == "takes") {
					QStringList parameters;

					do {
						JassToken type_name = tokenizer.eat_comment_blocks();
						JassToken identifier_name = tokenizer.eat_comment_blocks();

						parameters.append(type_name.value() + ' ' + identifier_name.value());

						token = tokenizer.eat_comment_blocks();
					} while (token.type() == TOKEN_OPERATOR);

					QString parameter_string = parameters.join(", ");
					QString declaration = value + '(' + parameter_string + ')';
					functions.insert(value);
					api.add(declaration);
				}
			}
		} else if (token.value() == "native") {
			token = tokenizer.eat_comment_blocks();

			if (token.type() == TOKEN_IDENTIFIER) {
				QString value = token.value();

				token = tokenizer.eat_comment_blocks();
				if (token.value() == "takes") {
					QStringList parameters;

					do {
						JassToken type_name = tokenizer.eat_comment_blocks();
						JassToken identifier_name = tokenizer.eat_comment_blocks();

						parameters.append(type_name.value() + ' ' + identifier_name.value());

						token = tokenizer.eat_comment_blocks();
					} while (token.type() == TOKEN_OPERATOR);

					QString parameter_string = parameters.join(", ");
					QString declaration = value + '(' + parameter_string + ')';
					natives.insert(value);
					api.add(declaration);
				}
			}
		} else if (token.value() == "type" || token.value() == "struct") {
			token = tokenizer.eat_comment_blocks();

			if (token.type() == TOKEN_IDENTIFIER) {
				types.insert(token.value());
				api.add(token.value());

				token = tokenizer.next();
			}
		} else if (token.value() == "constant") {
			token = tokenizer.eat_comment_blocks();

			if (token.value() != "function" && token.value() != "native" && types.contains(token.value())) {
				token = tokenizer.eat_comment_blocks();

				constants.insert(token.value());
				api.add(token.value());

				token = tokenizer.next();
			}
		} else {
			token = tokenizer.next();
		}
	}
	api.prepare();

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