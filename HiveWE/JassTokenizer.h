#pragma once

#include <QString>
#include <QList>

enum JassTokenType
{
	TOKEN_NUMBER,
	TOKEN_IDENTIFIER,
	TOKEN_COMMENT_START,
	TOKEN_PREPROCESSOR_COMMENT_START,
	TOKEN_NEWLINE,
	TOKEN_COMMENT_BLOCK_START,
	TOKEN_COMMENT_BLOCK_END,
	TOKEN_DOUBLE_QUOTE,
	TOKEN_SINGLE_QUOTE,
	TOKEN_OTHER,
	TOKEN_EOF
};

class JassToken
{
private:
	QString value_;
	int start_;
	int stop_;

	JassTokenType type_;

public:
	JassToken(QString value, int start, int stop, JassTokenType type);

	QString const &value() const;

	int start() const;
	int stop() const;

	JassTokenType type() const;

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