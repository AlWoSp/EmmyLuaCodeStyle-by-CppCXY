#pragma once

class LuaFormatOptions
{
public:
	// �ҷ������ַ��
	// ������Ȼʵ����
	bool UseTabIndent = false;

	int Indent = 4;
	// ��ֻ�ǳ�ʼ��ʱ��Ĭ��ѡ���linux����Ȼ���Ա��� \r\n
#ifdef _WINDOWS
	std::string LineSeperater = "\r\n";
#else
	std::string LineSeperater = "\r";
#endif
};
