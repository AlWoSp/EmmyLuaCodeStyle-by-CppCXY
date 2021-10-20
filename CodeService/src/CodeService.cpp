#include <iostream>
#include "LuaParser/LuaParser.h"
#include "Util/format.h"
#include "CodeService/LuaFormatter.h"

int main()
{
	std::string source = R"(
local aa       = 123 --jgoiwjgw
local dd  = 445   --0swgfwkgwp
	--fuck me
aa = 890  --12313
local c<cccc> =123
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

	auto ast = parser->GetAst();
	LuaFormatOptions options;
	LuaFormatter formatter(parser, options);
	formatter.BuildFormattedElement();
	std::cout << "��ʽ�����:\n";
	std::cout << formatter.GetFormattedText();

	return 0;
}
