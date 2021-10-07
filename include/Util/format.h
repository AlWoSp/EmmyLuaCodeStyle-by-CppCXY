#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <sstream>
#include <type_traits>

class FormatReplaceExpress
{
public:
	FormatReplaceExpress(std::string_view expr, std::size_t startIndex, std::size_t endIndex, bool needReplace)
		: Expr(expr),
		  StartIndex(startIndex),
		  EndIndex(endIndex),
		  NeedReplace(needReplace)
	{
	}

	std::string_view Expr;
	std::size_t StartIndex;
	std::size_t EndIndex;
	bool NeedReplace;
};

template <class T>
std::string toFormatString(T t)
{
	if constexpr (std::is_same_v<T, std::string_view>)
	{
		return std::string(t);
	}
	else if constexpr (std::is_same_v<T, std::string>)
	{
		return t;
	}
	else
	{
		return std::to_string(t);
	}
}


/*
 * ֱ������Ŀ����ʱ,gcc��Ȼû�����C++20 format ʵ��
 * �Լ�дһ��
 */
template <class... ARGS>
std::string format(std::string_view fmt, ARGS ... args)
{
	std::vector<std::string> argVec = {toFormatString<ARGS>(args)...};

	enum class ParseState
	{
		Normal,
		LeftBrace,
		RightBrace
	} state = ParseState::Normal;

	std::vector<FormatReplaceExpress> replaceExpresses;


	std::size_t leftBraceBegin = 0;

	std::size_t rightBraceBegin = 0;

	// ����ڱ��ʽ�г����������
	std::size_t exprLeftCount = 0;


	for (std::size_t index = 0; index != fmt.size(); index++)
	{
		char ch = fmt[index];

		switch (state)
		{
		case ParseState::Normal:
			{
				if (ch == '{')
				{
					state = ParseState::LeftBrace;
					leftBraceBegin = index;
					exprLeftCount = 0;
				}
				else if (ch == '}')
				{
					state = ParseState::RightBrace;
					rightBraceBegin = index;
				}
				break;
			}
		case ParseState::LeftBrace:
			{
				if (ch == '{')
				{
					// ��Ϊ����˫������ת��Ϊ�ɼ���'{'
					if (index == leftBraceBegin + 1)
					{
						replaceExpresses.emplace_back("{", leftBraceBegin, index, false);
						state = ParseState::Normal;
					}
					else
					{
						exprLeftCount++;
					}
				}
				else if (ch == '}')
				{
					// ��Ϊ�Ǳ��ʽ�ڵĴ�����
					if (exprLeftCount > 0)
					{
						exprLeftCount--;
						continue;
					}

					replaceExpresses.emplace_back(fmt.substr(leftBraceBegin + 1, index - leftBraceBegin - 1),
					                              leftBraceBegin, index, true);


					state = ParseState::Normal;
				}
				break;
			}
		case ParseState::RightBrace:
			{
				if (ch == '}' && (index == rightBraceBegin + 1))
				{
					replaceExpresses.emplace_back("}", rightBraceBegin, index, false);
				}
				else
				{
					//��Ϊ���Ҵ�����ʧ�䣬֮ǰ�Ĳ��������˸�һλ��ȥ�����ж�
					index--;
				}
				state = ParseState::Normal;
				break;
			}
		}
	}

	std::stringstream message;

	if (replaceExpresses.empty())
	{
		message << fmt;
	}
	else
	{
		// ƴ���ַ���
	std:size_t replaceCount = 0;
		std::size_t start = 0;
		for (std::size_t index = 0; index != replaceExpresses.size(); index++)
		{
			auto& replaceExpress = replaceExpresses[index];
			if (start < replaceExpress.StartIndex)
			{
				auto fragment = fmt.substr(start, replaceExpress.StartIndex - start);
				message << fragment;
				start = replaceExpress.StartIndex;
			}

			if (replaceExpress.NeedReplace)
			{
				if (replaceCount < argVec.size())
				{
					message << argVec[replaceCount++];
				}
			}
			else
			{
				message << replaceExpress.Expr;
			}

			start = replaceExpress.EndIndex + 1;
		}

		if (start < fmt.size())
		{
			auto fragment = fmt.substr(start, fmt.size() - start);
			message << fragment;
		}
	}
	return message.str();
}
