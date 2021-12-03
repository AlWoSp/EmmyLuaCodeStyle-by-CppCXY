#pragma once

#include <string>
#include <map>

class LanguageTranslator
{
public:
	static LanguageTranslator& GetInstance();

	LanguageTranslator();

	std::string Get(const std::string& source);

	void SetLanguageMap(std::map<std::string, std::string>&& languageDictionary);

private:
	std::map<std::string, std::string> _languageDictionary;
};

/*
 * ���Ա��ػ�����ʵ��������һ�ֹ��÷�
 */
#define LText(text) LanguageTranslator::GetInstance().Get(text)

