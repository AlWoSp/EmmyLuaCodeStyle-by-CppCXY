#include "CodeService/FormatElement/FormatContext.h"

FormatContext::FormatContext(std::shared_ptr<LuaParser> parser, LuaFormatOptions& options)
	: _options(options),
	  _characterCount(0),
	  _parser(parser)

{
}

void FormatContext::Print(std::string_view text)
{
	auto& indentState = _indentStack.top();
	if (static_cast<int>(_characterCount) < indentState.Indent)
	{
		PrintIndent(indentState.Indent - static_cast<int>(_characterCount), indentState.IndentString);
	}
	_os << text;
	_characterCount += text.size();
}

void FormatContext::PrintLine(int line)
{
	for (int i = 0; i < line; i++)
	{
		_os << _options.line_separator;
		_characterCount = 0;
	}
}

void FormatContext::PrintBlank(int blank)
{
	for (int i = 0; i < blank; i ++)
	{
		_os << ' ';
		_characterCount++;
	}
}

void FormatContext::PrintIndent(int indent, const std::string& indentString)
{
	// ��ʼ����Ϊʹ\t���Ű��������
	for (int i = 0; i < indent; i++)
	{
		_os << indentString;
		_characterCount += indentString.size();
	}
}

void FormatContext::AddIndent(int specialIndent)
{
	if (!_options.use_tab)
	{
		int newIndent = 0;
		if (specialIndent == -1)
		{
			if (_indentStack.empty())
			{
				_indentStack.push({0, ""});
				return;
			}

			auto& topIndent = _indentStack.top();
			newIndent = _options.indent + topIndent.Indent;
		}
		else
		{
			newIndent = specialIndent;
		}
		_indentStack.push({newIndent, " "});
	}
	else //����㷨���ܻ�������
	{
		int newIndent = 0;
		if (specialIndent == -1)
		{
			if (_indentStack.empty())
			{
				_indentStack.push({0, ""});
				return;
			}

			auto& topIndent = _indentStack.top();
			// һ��ֻ��һ��\t
			newIndent = 1 + topIndent.Indent;
		}
		else
		{
			// �һ���Ϊ����һ������
			// �����ƶ���һ�����������һ���Ϊ������һ������
			newIndent = std::max(1, specialIndent / 8);
		}
		std::string indentString = "\t";
		_indentStack.push({newIndent, "\t"});
	}
}

void FormatContext::RecoverIndent()
{
	_indentStack.pop();
}

int FormatContext::GetLine(int offset)
{
	return _parser->GetLine(offset);
}

int FormatContext::GetColumn(int offset)
{
	return _parser->GetColumn(offset);
}

std::size_t FormatContext::GetCharacterCount() const
{
	return _characterCount;
}

std::size_t FormatContext::GetCurrentIndent() const
{
	if (_indentStack.empty())
	{
		return 0;
	}

	return _indentStack.top().Indent;
}

std::string FormatContext::GetText()
{
	return _os.str();
}

std::shared_ptr<LuaParser> FormatContext::GetParser()
{
	return _parser;
}
