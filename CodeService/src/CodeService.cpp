#include <iostream>
#include "LuaParser/LuaParser.h"
#include "Util/format.h"
#include "CodeService/LuaFormatter.h"

int main()
{
	std::string source = R"(#! fuck
local t,b =123
)";
	std::cout << "ԭ��:\n" << source;
	auto parser = LuaParser::LoadFromBuffer(std::move(source));

	parser->BuildAstWithComment();

	auto errors = parser->GetErrors();

	for (auto& err : errors)
	{
		std::cout << format("error: {} , textRange({},{})", err.ErrorMessage, err.ErrorRange.StartOffset,
		                    err.ErrorRange.EndOffset) << std::endl;
	}

	// auto comments = parser->GetAllComments();
	//
	// for (auto& comment : comments)
	// {
	// 	std::cout << format("comment is \n{}\n", comment.Text);
	// }

	// auto ast = parser->GetAst();
	LuaFormatOptions options;
	LuaFormatter formatter(parser, options);
	formatter.BuildFormattedElement();
	std::cout << "��ʽ�����:\n";
	std::cout << formatter.GetFormattedText();

	return 0;
}
