#include "LuaParser/LuaAstNode.h"
#include "LuaParser/LuaParseException.h"

LuaAstNode::LuaAstNode(LuaAstNodeType type, const char* source)
	: _type(type),
	  _parent(),
	  _source(source),
	  _text(source, 0)
{
}

LuaAstNode::LuaAstNode(LuaAstNodeType type, LuaToken& token)
	: LuaAstNode(type, nullptr)
{
	auto text = token.Text;
	const char* source = text.data() - token.TextRange.StartOffset;
	_source = source;
	_textRange = token.TextRange;
	_text = token.Text;
}

TextRange LuaAstNode::GetTextRange() const
{
	return _textRange;
}

std::string_view LuaAstNode::GetText() const
{
	return _text;
}

const std::vector<std::shared_ptr<LuaAstNode>>& LuaAstNode::GetChildren() const
{
	return _children;
}

void LuaAstNode::AddChild(std::shared_ptr<LuaAstNode> child)
{
	if (child == nullptr)
	{
		return;
	}

	if (child->_source != _source)
	{
		throw LuaParserException("child source not match parent source");
	}

	if (_textRange.StartOffset == 0 && _textRange.EndOffset == 0)
	{
		_textRange = child->_textRange;
	}

	else
	{
		if (child->_textRange.StartOffset == 0 && child->_textRange.EndOffset == 0)
		{
			return;
		}

		if (_textRange.StartOffset > child->_textRange.StartOffset)
		{
			_textRange.StartOffset = child->_textRange.StartOffset;
		}

		if (_textRange.EndOffset < child->_textRange.EndOffset)
		{
			_textRange.EndOffset = child->_textRange.EndOffset;
		}

		
	}
	_children.push_back(child);
	_text = std::string_view(_source + _textRange.StartOffset, _textRange.EndOffset - _textRange.StartOffset + 1);
}

LuaAstNodeType LuaAstNode::GetType() const
{
	return _type;
}

void LuaAstNode::SetType(LuaAstNodeType type)
{
	_type = type;
}

void LuaAstNode::SetError(std::string_view errorMessage)
{
}
