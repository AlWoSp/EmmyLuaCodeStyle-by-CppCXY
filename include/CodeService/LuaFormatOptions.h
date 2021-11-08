#pragma once

class LuaFormatOptions
{
public:
	/*
	 * �ҷ������ַ��
	 * ������Ȼʵ����
	 */ 
	bool UseTabIndent = false;

	/*
	 * �����Ŀհ���
	 */ 
	int Indent = 4;

	/*
	 * ���ò������뵽��һ������������ʵ���������ַ����vscode�ϻ���Ϊ��ֱ�����ߵı�ע���Եü�Ϊ�ѿ�
	 * 
	 */
	bool AlignCallArgs = false;

	/*
	 * �������õ����źͲ���֮�䱣��һ���ո�
	 *
	 */
	bool BlankBetweenCallArgsAndBracket = false;

	/*
	 * ��������Ĳ������ֶ��뵽��һ������
	 * �����Ķ���ͨ������̫��������Ĭ����Ϊ�ǿ��Խ��ܵ�
	 */
	bool AlignFunctionDefineParams = true;

	/*
	 * ��Ĵ����źͱ���֮�䱣��һ���ո�
	 * ����ʵ���������ַ�ʽ�Ƚ�����
	 */
	bool BlankBetweenTableFieldsAndBracket = true;

	/*
	 * ����ÿһ��������뵽��һ������
	 */
	bool AlignTableFieldToFirst = true;

	/*
	 * ���̫����
	 */
	// bool KeepSingleQuote = false;
	// bool KeepDoubleQuote = false;
	// bool KeepTableFieldSepComma = false;
	// bool KeepTableFieldSepSemicolon = false;

	/*
	 * ���������������һ�����֮�䱣����С1��
	 */
	int BlockStatementKeepMinLine = 1;

	// ��ֻ�ǳ�ʼ��ʱ��Ĭ��ѡ���linux����Ȼ���Ա��� \r\n
#ifdef _WINDOWS
	std::string LineSeperater = "\r\n";
#else
	std::string LineSeperater = "\n";
#endif
};
