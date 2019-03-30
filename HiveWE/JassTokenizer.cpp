#include "stdafx.h"
#include "JassTokenizer.h"

JassToken::JassToken(QString value, int start, int stop, JassTokenType type) :
	value_(std::move(value)),
	start_(start),
	stop_(stop),
	type_(type)
{

}

QString const &JassToken::value() const
{
	return value_;
}

int JassToken::start() const
{
	return start_;
}

int JassToken::stop() const
{
	return stop_;
}

JassTokenType JassToken::type() const
{
	return type_;
}

int JassToken::length() const
{
	return stop() - start();
}

JassTokenizer::JassTokenizer(QString const &str)
{
	// TODO@Daniel:
	// Maybe an easier to read equivalent would be sufficient
	int start = 0;
	while (start < str.size())
	{
		int stop = start + 1;

		JassTokenType type = TOKEN_OTHER;

		switch (str[start].toLatin1())
		{
		case '/':
			if (start + 1 >= str.size())
			{
				break;
			}

			switch (str[start + 1].toLatin1())
			{
			case '/':
				if (start + 2 < str.size() && str[start + 2] == '!')
				{
					type = TOKEN_PREPROCESSOR_COMMENT_START;
					stop = start + 3;
				}
				else
				{
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
			if (start + 1 < str.size() && str[start + 1] == '/')
			{
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
			if (start + 1 < str.size() && str[start + 1] == '\n')
			{
				stop = start + 2;
			}
			else
			{
				stop = start + 1;
			}
			break;
		default:
			if (str[start].isLetter())
			{
				type = TOKEN_IDENTIFIER;
				while (stop < str.size() && str[stop].isLetter())
				{
					stop++;
				}
			}
			else if (str[start].isDigit())
			{
				type = TOKEN_NUMBER;
				while (stop < str.size() && str[stop].isDigit())
				{
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

int JassTokenizer::count() const
{
	return tokens_.count();
}

JassToken const &JassTokenizer::operator[](int idx) const
{
	return tokens_[std::min(idx, tokens_.size() - 1)];
}
