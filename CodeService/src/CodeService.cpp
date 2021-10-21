#include <iostream>
#include "LuaParser/LuaParser.h"
#include "Util/format.h"
#include "CodeService/LuaFormatter.h"
#include <fstream>

int main()
{
		std::string source = R"(
local t = 123
print(t)

---@class ccc :
local t = {
	--xixixi
	---hofowjfpw
	--wl;jgfo2jg
	deee  = "1231", 


    eeee = 12313131, --mfuck
    --hhoh

    __12313ffwfw = 1231
}
	
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
	std::cout << "\n��ʽ�����:\n";
	std::cout << formatter.GetFormattedText();

	// std::ofstream f(R"(C:\Users\zc\Desktop\learn\UI\UILogin\UILoginReference2.lua)", std::ios::out);

	// auto s = formatter.GetFormattedText();
	// f.write(s.c_str(), s.size());

	return 0;
}
