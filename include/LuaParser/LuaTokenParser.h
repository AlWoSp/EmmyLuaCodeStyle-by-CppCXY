#pragma once

#include <map>
#include <string_view>
#include <string>
#include <vector>
#include <set>

#include "LuaToken.h"
#include "LuaTokenType.h"
#include "LuaParseException.h"

/*
 * token ����������lua Դ����,ʵ���Ϸǳ��ӽ���ϸ�ڴ�������ͬ
 */
class LuaTokenParser
{
public:
	LuaTokenParser(std::string&& source);

	bool Parse();

	LuaToken& Next();

	LuaToken& LookAhead();

	LuaToken& Current();

	int GetLine(int offset);
	
	int GetColumn(int offset);

	std::string& GetSource();

	std::vector<LuaToken> GetComments();

private:
	static std::map<std::string, LuaTokenType, std::less<>> LuaReserved;

	LuaTokenType llex();

	int nextChar();

	void saveAndNext();

	void save();

	int getCurrentChar();

	bool checkNext1(int ch);

	bool checkNext2(std::string_view set);

	LuaTokenType readNumeral();

	std::size_t skipSep();

	void readLongString(std::size_t sep);

	void readString(int del);

	void incLinenumber();

	bool currentIsNewLine();

	void resetBuffer();

	std::string_view getSaveText() const;

	bool isReserved(std::string_view text);

	int _linenumber;

	bool _hasSaveText;
	std::size_t _buffStart;
	std::size_t _buffIndex;

	bool _hasEoz;
	std::size_t _currentParseIndex;
	std::size_t _currentIndex;
	std::string _source;

	std::vector<LuaToken> _tokens;
	// ע�͵�������
	std::vector<LuaToken> _commentTokens;

	LuaToken _eosToken;

	std::vector<int> _lineOffsetVec;
};
