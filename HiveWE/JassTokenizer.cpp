#include "stdafx.h"
#include "JassTokenizer.h"

JassToken::JassToken(QString value, int start, int stop, JassTokenType type, QList<JassToken> nested_tokens) :
	value_(std::move(value)),
	start_(start),
	stop_(stop),
	type_(type),
	nested_tokens_(std::move(nested_tokens)) {

}


QString const& JassToken::value() const {
	return value_;
}

int JassToken::start() const {
	return start_;
}

int JassToken::stop() const {
	return stop_;
}

JassTokenType JassToken::type() const {
	return type_;
}

QList<JassToken> const& JassToken::nested_tokens() const
{
	return nested_tokens_;
}

int JassToken::length() const {
	return stop() - start();
}

JassTokenizer::JassTokenizer(QString const &str) {
JassToken JassTokenizer::eat_comment_block()
{
	JassTokenType type = TOKEN_COMMENT_BLOCK;
	int stop = idx_;
	if (stop + 2 < text_size() && text_[stop] == '/' && text_[stop + 1] == '*')
	{
		stop += 2;
	}

	while (stop + 1 < text_size())
	{
		// TODO@Daniel:
		// Handle docstring parameters
		if (text_[stop] == '*' && text_[stop + 1] == '/')
		{
			stop++;
			break;
		}
		else
		{
			stop++;
		}
	}
	stop++;

	QString value = text_.mid(idx_, stop - idx_);
	JassToken token(value, idx_, stop, type);

	idx_ = stop;

	return token;
}

JassToken JassTokenizer::eat_string()
{
	QString slash_escapes = "\\nt\"";
	QString pipe_escapes = "cnr";

	JassTokenType type = TOKEN_STRING;
	QList<JassToken> nested_tokens;

	int stop = idx_;
	if (stop < text_size() && text_[stop] == '"')
	{
		stop++;
	}

	while (stop < text_size())
	{
		if (text_[stop] == '"')
		{
			stop++;
			break;
		}

		if (stop + 1 < text_.size())
		{
			// Iirc, there are only single character escapes
			bool is_slash_escape = text_[stop] == '\\' && slash_escapes.contains(text_[stop + 1]);
			bool is_pipe_escape = text_[stop] == '|' && pipe_escapes.contains(text_[stop + 1]);
			if (is_pipe_escape || is_slash_escape)
			{
				QString value = text_.mid(stop, stop + 2);
				JassToken token(value, stop, stop + 2, TOKEN_ESCAPE_SEQUENCE);
				nested_tokens.append(token);

				stop++;
			}
		}
		stop++;
	}
	stop++;

	QString value = text_.mid(idx_, stop - idx_);
	JassToken token(value, idx_, stop, type, nested_tokens);

	idx_ = stop;

	return token;
}

JassToken JassTokenizer::eat_rawcode()
{
	JassTokenType type = TOKEN_RAWCODE;
	int stop = idx_;
	if (stop < text_size() && text_[stop] == '\'')
	{
		stop++;
	}

	while (stop < text_size())
	{
		// Iirc, there are no escape sequences in rawcodes
		if (text_[stop] == '\'')
		{
			stop++;
			break;
		}
		else
		{
			stop++;
		}
	}

	QString value = text_.mid(idx_, stop - idx_);
	JassToken token(value, idx_, stop, type);

	idx_ = stop;

	return token;
}


	JassTokenType type = TOKEN_OTHER;

		// TODO@Daniel:
		// Properly handle whitespace
		switch (str[start].toLatin1()) {
	case '/':
			if (start + 1 >= str.size()) {
			break;
		}

			switch (str[start + 1].toLatin1()) {
		case '/':
				if (start + 2 < str.size() && str[start + 2] == '!') {
					type = TOKEN_PREPROCESSOR_COMMENT_START;
					stop = start + 3;
			}
			else {
					type = TOKEN_COMMENT_START;
					stop = start + 2;
			}
				break;
			case '*':
				type = TOKEN_COMMENT_BLOCK_START;
				stop = start + 2;
				break;
			}
			break;
		case '*':
			if (start + 1 < str.size() && str[start + 1] == '/') {
				type = TOKEN_COMMENT_BLOCK_END;
				stop = start + 2;
		}
		break;
	case '"':
			type = TOKEN_DOUBLE_QUOTE;
			stop = start + 1;
			break;
	case '\'':
			type = TOKEN_SINGLE_QUOTE;
			stop = start + 1;
			break;
	case '\n':
		type = TOKEN_NEWLINE;
			stop = start + 1;
		break;
	case '\r':
		type = TOKEN_NEWLINE;
			if (start + 1 < str.size() && str[start + 1] == '\n') {
				stop = start + 2;
		}
		else {
				stop = start + 1;
		}
		break;
	default:
			if (str[start].isLetter() || str[start] == '_') {
			type = TOKEN_IDENTIFIER;
				while (stop < str.size() && (str[stop].isLetter() || str[stop].isDigit() || str[stop] == '_')) {
				stop++;
			}
		}
			else if (str[start].isDigit()) {
			type = TOKEN_NUMBER;
				while (stop < str.size() && str[stop].isDigit()) {
				stop++;
			}
		}
		break;
	}

		QString value = str.mid(start, stop - start);

		JassToken token(value, start, stop, type);
		tokens_.append(token);

		start = stop;
	}

	JassToken eof("", str.size(), str.size(), TOKEN_EOF);
	tokens_.append(eof);
}

int JassTokenizer::count() const {
	return tokens_.count();
}

JassToken const &JassTokenizer::operator[](int idx) const {
	return tokens_[std::min(idx, tokens_.size() - 1)];
}
