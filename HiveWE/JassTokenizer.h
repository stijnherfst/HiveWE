#pragma once

#pragma warning(push, 0)
#include <QChar>
#include <QString>
#include <QList>
#pragma warning(pop) 

enum JassTokenType {
	TOKEN_NUMBER,
	TOKEN_IDENTIFIER,
	TOKEN_COMMENT_LINE,
	TOKEN_PREPROCESSOR_COMMENT,
	TOKEN_NEWLINE,
	TOKEN_COMMENT_BLOCK,
	// TODO@Daniel:
	// Ideally, operators should each have their own token type
	TOKEN_OPERATOR,
	TOKEN_STRING,
	TOKEN_RAWCODE,
	TOKEN_ESCAPE_SEQUENCE,
	TOKEN_WHITESPACE,
	TOKEN_OTHER,
	TOKEN_EOF
};

class JassToken {
private:
	QString value_;
	int start_;
	int stop_;

	JassTokenType type_;

	// For escape sequences in strings, parameters in docstring and similar
	QList<JassToken> nested_tokens_;

public:
	JassToken(QString value, int start, int stop, JassTokenType type, QList<JassToken> nested_tokens = QList<JassToken>());

	QString const& value() const;

	int start() const;
	int stop() const;

	JassTokenType type() const;

	QList<JassToken> const& nested_tokens() const;

	int length() const;
};

// TODO@Daniel:
// Add iterator for convenience
class JassTokenizer {
private:
	// TODO@Daniel:
	// Support multiple 'files' instead of just one
	// Will help with block comments across files and such
	QString text_;

	int idx_;

	QChar at(int idx) const;

public:
	JassTokenizer(QString str);

	int text_size() const;

	JassToken parse_comment_block();
	JassToken parse_string();
	JassToken parse_rawcode();

	JassToken eat_all_of(JassTokenType token_type);
	JassToken eat_until(JassTokenType token_type);
	JassToken eat_comment_blocks();

	JassToken next();
};