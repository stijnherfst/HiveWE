#pragma once

#include <QString>
#include <QList>

enum JassTokenType {
	TOKEN_NUMBER,
	TOKEN_IDENTIFIER,
	TOKEN_COMMENT_LINE,
	TOKEN_PREPROCESSOR_COMMENT,
	TOKEN_NEWLINE,
	TOKEN_COMMENT_BLOCK,
	TOKEN_COMMENT_BLOCK_END,
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

class JassTokenizer {
private:
	QList<JassToken> tokens_;

public:
	JassTokenizer(QString const &str);

	int count() const;

	JassToken const &operator[](int idx) const;
};