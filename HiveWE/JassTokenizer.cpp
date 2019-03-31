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

JassTokenizer::JassTokenizer(QString str) :
	text_(std::move(str)),
	idx_(0) {
}

int JassTokenizer::text_size() const {
	return text_.size();
}

JassToken JassTokenizer::parse_comment_block() {
	JassTokenType type = TOKEN_COMMENT_BLOCK;
	int stop = idx_;
	if (stop + 2 < text_size() && text_[stop] == '/' && text_[stop + 1] == '*') {
		stop += 2;
	}

	while (stop + 1 < text_size()) {
		// TODO@Daniel:
		// Handle docstring parameters
		if (text_[stop] == '*' && text_[stop + 1] == '/') {
			stop++;
			break;
		}
		else {
			stop++;
		}
	}
	stop++;

	QString value = text_.mid(idx_, stop - idx_);
	JassToken token(value, idx_, stop, type);

	idx_ = stop;

	return token;
}

JassToken JassTokenizer::parse_string() {
	QString slash_escapes = "\\nt\"";
	QString pipe_escapes = "cnr";

	JassTokenType type = TOKEN_STRING;
	QList<JassToken> nested_tokens;

	int stop = idx_;
	if (stop < text_size() && text_[stop] == '"') {
		stop++;
	}

	while (stop < text_size()) {
		if (text_[stop] == '"') {
			stop++;
			break;
		}

		if (stop + 1 < text_size()) {
			// Iirc, there are only single character escapes
			bool is_slash_escape = text_[stop] == '\\' && slash_escapes.contains(text_[stop + 1]);
			bool is_pipe_escape = text_[stop] == '|' && pipe_escapes.contains(text_[stop + 1]);
			if (is_pipe_escape || is_slash_escape) {
				QString value = text_.mid(stop, stop + 2);
				JassToken token(value, stop, stop + 2, TOKEN_ESCAPE_SEQUENCE);
				nested_tokens.append(token);

				stop++;
			}
		}
		stop++;
	}

	QString value = text_.mid(idx_, stop - idx_);
	JassToken token(value, idx_, stop, type, nested_tokens);

	idx_ = stop;

	return token;
}

JassToken JassTokenizer::parse_rawcode() {
	JassTokenType type = TOKEN_RAWCODE;
	int stop = idx_;
	if (stop < text_size() && text_[stop] == '\'') {
		stop++;
	}

	while (stop < text_size()) {
		// Iirc, there are no escape sequences in rawcodes
		if (text_[stop] == '\'') {
			stop++;
			break;
		}
		else {
			stop++;
		}
	}

	QString value = text_.mid(idx_, stop - idx_);
	JassToken token(value, idx_, stop, type);

	idx_ = stop;

	return token;
}

JassToken JassTokenizer::eat_all_of(JassTokenType token_type) {
	JassToken token = next();
	while (token.type() == token_type) {
		token = next();
	}
	return token;
}

JassToken JassTokenizer::next() {
	// Skipping leading whitespace
	while (idx_ < text_size() && text_[idx_].isSpace() && text_[idx_] != '\r' && text_[idx_] != '\n') {
		idx_++;
	}

	if (idx_ >= text_size()) {
		return JassToken("", text_size(), text_size(), TOKEN_EOF);
	}

	int stop = idx_ + 1;

	JassTokenType type = TOKEN_OTHER;

	QList<JassToken> nested_tokens;

	switch (text_[idx_].toLatin1()) {
	case '/':
		if (idx_ + 1 >= text_size()) {
			break;
		}

		switch (text_[idx_ + 1].toLatin1()) {
		case '/':
			// Technically, changing stop here isn't needed but keeps it somewhat consistent
			if (idx_ + 2 < text_size() && text_[idx_ + 2] == '!') {
				type = TOKEN_PREPROCESSOR_COMMENT;
				stop = idx_ + 3;
			}
			else {
				type = TOKEN_COMMENT_LINE;
				stop = idx_ + 2;
			}

			while (stop < text_size() && text_[stop] != '\r' && text_[stop] != '\n') {
				stop++;
			}
			break;
		case '*':
			return parse_comment_block();
		}
		break;
	case '"':
		return parse_string();
	case '\'':
		return parse_rawcode();
	case '\n':
		type = TOKEN_NEWLINE;
		stop = idx_ + 1;
		break;
	case '\r':
		type = TOKEN_NEWLINE;
		if (idx_ + 1 < text_size() && text_[idx_ + 1] == '\n') {
			stop = idx_ + 2;
		}
		else {
			stop = idx_ + 1;
		}
		break;
	case ',':
	case '.':
	case '[':
	case ']':
	case '(':
	case ')':
		type = TOKEN_OPERATOR;
		stop = idx_ + 1;
		break;
	default:
		// TODO@Daniel:
		// Move these out since they will be needed elsewhere as well
		if (text_[idx_].isLetter() || text_[idx_] == '_') {
			type = TOKEN_IDENTIFIER;
			while (stop < text_size() && (text_[stop].isLetter() || text_[stop].isDigit() || text_[stop] == '_')) {
				stop++;
			}
		}
		else if (text_[idx_].isDigit()) {
			type = TOKEN_NUMBER;
			while (stop < text_size() && text_[stop].isDigit()) {
				stop++;
			}
		}
		break;
	}

	QString value = text_.mid(idx_, stop - idx_);

	JassToken token(value, idx_, stop, type, nested_tokens);
	idx_ = stop;

	return token;
}
