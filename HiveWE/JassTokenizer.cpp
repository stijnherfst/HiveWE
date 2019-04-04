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

QList<JassToken> const& JassToken::nested_tokens() const {
	return nested_tokens_;
}

int JassToken::length() const {
	return stop() - start();
}

QChar JassTokenizer::at(int idx) const {
	if (idx < 0 || idx >= text_size()) {
		return '\0';
	}

	return text_[idx];
}

JassTokenizer::JassTokenizer(QString str) :
	text_(std::move(str)),
	idx_(0) {
}

int JassTokenizer::text_size() const {
	return text_.size();
}

JassToken JassTokenizer::parse_comment_block() {
	int stop = idx_;
	if (at(stop) == '/' && at(stop + 1) == '*') {
		stop += 2;
	}

	while (stop + 1 < text_size()) {
		// TODO@Daniel:
		// Handle docstring parameters
		if (at(stop) == '*' && at(stop + 1) == '/') {
			stop++;
			break;
		} else {
			stop++;
		}
	}
	stop++;

	QString value = text_.mid(idx_, stop - idx_);
	JassToken token(value, idx_, stop, TOKEN_COMMENT_BLOCK);

	idx_ = stop;

	return token;
}

JassToken JassTokenizer::parse_string() {
	QString slash_escapes = "\\nt\"";
	QString pipe_escapes = "cnr";

	QList<JassToken> nested_tokens;

	int stop = idx_;
	if (at(stop) == '"') {
		stop++;
	}

	while (stop < text_size()) {
		if (at(stop) == '"') {
			stop++;
			break;
		}

		// Iirc, there are only single character escapes
		bool is_slash_escape = at(stop) == '\\' && slash_escapes.contains(at(stop + 1));
		bool is_pipe_escape = at(stop) == '|' && pipe_escapes.contains(at(stop + 1));
		if (is_pipe_escape || is_slash_escape) {
			QString value = text_.mid(stop, stop + 2);
			JassToken token(value, stop, stop + 2, TOKEN_ESCAPE_SEQUENCE);
			nested_tokens.append(token);

			stop++;
		}
		stop++;
	}

	QString value = text_.mid(idx_, stop - idx_);
	JassToken token(value, idx_, stop, TOKEN_STRING, nested_tokens);

	idx_ = stop;

	return token;
}

JassToken JassTokenizer::parse_rawcode() {
	int stop = idx_;
	if (at(stop) == '\'') {
		stop++;
	}

	while (stop < text_size()) {
		// Iirc, there are no escape sequences in rawcodes
		if (at(stop) == '\'') {
			stop++;
			break;
		} else {
			stop++;
		}
	}

	QString value = text_.mid(idx_, stop - idx_);
	JassToken token(value, idx_, stop, TOKEN_RAWCODE);

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

JassToken JassTokenizer::eat_until(JassTokenType token_type) {
	JassToken token = next();
	while (token.type() != token_type) {
		token = next();
	}
	return token;
}

JassToken JassTokenizer::eat_comment_blocks() {
	return eat_all_of(TOKEN_COMMENT_BLOCK);
}

JassToken JassTokenizer::next() {
	// Skipping leading whitespace
	while (at(idx_).isSpace() && at(idx_) != '\r' && at(idx_) != '\n') {
		idx_++;
	}

	if (idx_ >= text_size()) {
		return JassToken("", text_size(), text_size(), TOKEN_EOF);
	}

	int stop = idx_ + 1;

	JassTokenType type = TOKEN_OTHER;

	QList<JassToken> nested_tokens;

	switch (at(idx_).toLatin1()) {
		case '/':
			if (idx_ + 1 >= text_size()) {
				break;
			}

			switch (at(idx_ + 1).toLatin1()) {
				case '/':
					// Technically, changing stop here isn't needed but keeps it somewhat consistent
					if (at(idx_ + 2) == '!') {
						type = TOKEN_PREPROCESSOR_COMMENT;
						stop = idx_ + 3;
					} else {
						type = TOKEN_COMMENT_LINE;
						stop = idx_ + 2;
					}

					while (at(stop) != '\r' && at(stop) != '\n') {
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
			if (at(idx_ + 1) == '\n') {
				stop = idx_ + 2;
			} else {
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
			if (at(idx_).isLetter() || at(idx_) == '_') {
				type = TOKEN_IDENTIFIER;
				while (at(stop).isLetter() || at(stop).isDigit() || at(stop) == '_') {
					stop++;
				}
			} else if (at(idx_).isDigit()) {
				type = TOKEN_NUMBER;
				while (at(stop).isDigit()) {
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
