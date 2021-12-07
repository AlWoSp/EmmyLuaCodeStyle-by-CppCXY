﻿#include "CodeService/LuaEditorConfig.h"
#include <sstream>
#include <fstream>
#include <vector>
#include <regex>
#include <filesystem>
#include "Util/StringUtil.h"
#include "CodeService/FormatElement/KeepElement.h"
#include "CodeService/FormatElement/MinLineElement.h"

std::string dirPath(std::string& filePath)
{
	for (int i = filePath.size() - 1; i >= 0; i--)
	{
		char ch = filePath[i];
		if(ch == '/' || ch == '\\')
		{
			return filePath.substr(0, i + 1);
		}
	}
	return "";
}

std::shared_ptr<LuaEditorConfig> LuaEditorConfig::LoadFromFile(const std::string& path)
{
	std::fstream fin(path, std::ios::in);
	if (fin.is_open())
	{
		std::stringstream s;
		s << fin.rdbuf();
		auto editorConfig = std::make_shared<LuaEditorConfig>(s.str());
		editorConfig->Parse();
		return editorConfig;
	}

	return nullptr;
}

LuaEditorConfig::LuaEditorConfig(std::string&& source)
	:  _source(source)
{
}

void LuaEditorConfig::Parse()
{
	auto lines = StringUtil::Split(_source, "\n");

	bool luaSectionFounded = false;
	std::regex comment = std::regex(R"(^\s*(;|#))");
	std::regex luaSection = std::regex(R"(^\s*\[\s*([^\]]+)\s*\]\s*$)");
	std::regex valueRegex = std::regex(R"(^\s*([\w\d_]+)\s*=\s*(.+)$)");

	std::shared_ptr<Section> currentSection = nullptr;
	for (auto& lineView : lines)
	{
		std::string line(lineView);
		if (std::regex_search(line, comment))
		{
			continue;
		}

		std::smatch m;

		if (std::regex_search(line, m, luaSection))
		{
			auto section = m.str(1);
			luaSectionFounded = (section.find("lua") != std::string::npos) || section == "*";
			currentSection = std::make_shared<Section>();
			_sectionMap.insert({section, currentSection});
			continue;
		}

		if (luaSectionFounded)
		{
			if (std::regex_search(line, m, valueRegex))
			{
				currentSection->ConfigMap.insert({m.str(1), std::string(StringUtil::TrimSpace(m.str(2)))});
			}
		}
	}
}

std::shared_ptr<LuaCodeStyleOptions> LuaEditorConfig::Generate(std::string_view fileUri)
{
	auto options = std::make_shared<LuaCodeStyleOptions>();

	std::string patternKey;
	std::vector<std::shared_ptr<Section>> luaSections;
	patternKey.reserve(64);

	// 这里可以用views ，但是github action可能支持不完C++20
	for (auto& [sectionPattern, section] : _sectionMap)
	{
		// [*] [*.lua] [*.{lua,js,ts}]
		if (sectionPattern == "*" || sectionPattern == "*.lua" || sectionPattern.starts_with("*.{"))
		{
			patternKey.append("#").append(sectionPattern);
			luaSections.push_back(section);
		}
			// [{test.lua,lib.lua}]
		else if (sectionPattern.starts_with("{") && sectionPattern.ends_with("}"))
		{
			auto fileListText = sectionPattern.substr(1, sectionPattern.size() - 2);
			auto fileList = StringUtil::Split(fileListText, ",");
			for (auto fileMatchUri : fileList)
			{
				if (fileUri.ends_with(StringUtil::TrimSpace(fileMatchUri)))
				{
					patternKey.append("#").append(sectionPattern);
					luaSections.push_back(section);
					break;
				}
			}
		}
			// [lib/**.lua]
		else if (sectionPattern.ends_with("**.lua"))
		{
			std::string prefix = sectionPattern.substr(0, sectionPattern.size() - 6);
			std::filesystem::path workspace(_workspace);
			
			auto dirname = workspace / prefix;
			std::filesystem::path file(fileUri);
			auto dirNormal = dirname.lexically_normal();
			auto fileNormal = file.lexically_normal();
			if (fileNormal.string().starts_with(dirNormal.string()))
			{
				patternKey.append("#").append(sectionPattern);
				luaSections.push_back(section);
			}
		}
			//[aaa/bbb.lua]
		else
		{
			std::filesystem::path workspace(_workspace);
			auto fileName = workspace / sectionPattern;
			if (fileUri == fileName)
			{
				patternKey.append("#").append(sectionPattern);
				luaSections.push_back(section);
			}
		}
	}

	if (_optionPatternMap.count(patternKey) > 0)
	{
		return _optionPatternMap[patternKey];
	}
	else
	{
		for (auto luaSection : luaSections)
		{
			ParseFromSection(options, luaSection->ConfigMap);
		}

		_optionPatternMap.insert({patternKey, options});
		return options;
	}
}

void LuaEditorConfig::SetWorkspace(std::string_view workspace)
{
	_workspace = std::string(workspace);
}

void LuaEditorConfig::ParseFromSection(std::shared_ptr<LuaCodeStyleOptions> options,
                                       std::map<std::string, std::string, std::less<>>& configMap)
{
	if (configMap.count("indent_style"))
	{
		if (configMap.at("indent_style") == "space")
		{
			options->indent_style = IndentStyle::Space;
		}
		else if (configMap.at("indent_style") == "tab")
		{
			options->indent_style = IndentStyle::Tab;
		}
	}

	if (configMap.count("indent_size"))
	{
		options->indent_size = std::stoi(configMap.at("indent_size"));
	}

	if (configMap.count("tab_width"))
	{
		options->tab_width = std::stoi(configMap.at("tab_width"));
	}

	if (configMap.count("continuation_indent_size"))
	{
		options->continuation_indent_size = std::stoi(configMap.at("continuation_indent_size"));
	}

	if (configMap.count("align_call_args"))
	{
		options->align_call_args = configMap.at("align_call_args") == "true";
	}

	if (configMap.count("keep_one_space_between_call_args_and_bracket"))
	{
		options->keep_one_space_between_call_args_and_bracket =
			configMap.at("keep_one_space_between_call_args_and_bracket") == "true";
	}

	if (configMap.count("keep_one_space_between_table_and_bracket"))
	{
		options->keep_one_space_between_table_and_bracket =
			configMap.at("keep_one_space_between_table_and_bracket") == "true";
	}

	if (configMap.count("align_table_field_to_first_field"))
	{
		options->align_table_field_to_first_field = configMap.at("align_table_field_to_first_field") == "true";
	}

	if (configMap.count("continuous_assign_statement_align_to_equal_sign"))
	{
		options->continuous_assign_statement_align_to_equal_sign =
			configMap.at("continuous_assign_statement_align_to_equal_sign") == "true";
	}

	if (configMap.count("continuous_assign_table_field_align_to_equal_sign"))
	{
		options->continuous_assign_table_field_align_to_equal_sign =
			configMap.at("continuous_assign_table_field_align_to_equal_sign") == "true";
	}

	if (configMap.count("end_of_line"))
	{
		auto lineSeparatorSymbol = configMap.at("end_of_line");
		if (lineSeparatorSymbol == "crlf")
		{
			options->end_of_line = "\r\n";
		}
		else if (lineSeparatorSymbol == "lf")
		{
			options->end_of_line = "\n";
		}
	}

	if (configMap.count("max_line_length"))
	{
		options->max_line_length = std::stoi(configMap.at("max_line_length"));
	}

	if (configMap.count("enable_check_codestyle"))
	{
		options->enable_check_codestyle = configMap.at("enable_check_codestyle") == "true";
	}

	std::vector<std::pair<std::string, std::shared_ptr<FormatElement>&>> fieldList = {
		{"keep_line_after_if_statement", options->keep_line_after_if_statement},
		{"keep_line_after_do_statement", options->keep_line_after_do_statement},
		{"keep_line_after_while_statement", options->keep_line_after_while_statement},
		{"keep_line_after_repeat_statement", options->keep_line_after_repeat_statement},
		{"keep_line_after_for_statement", options->keep_line_after_for_statement},
		{"keep_line_after_local_or_assign_statement", options->keep_line_after_local_or_assign_statement},
		{"keep_line_after_function_define_statement", options->keep_line_after_function_define_statement}
	};
	std::regex minLineRegex = std::regex(R"(minLine:\s*(\d+))");
	std::regex keepLineRegex = std::regex(R"(keepLine:\s*(\d+))");
	for (auto& keepLineOption : fieldList)
	{
		if (configMap.count(keepLineOption.first))
		{
			std::string value = configMap.at(keepLineOption.first);
			if (value == "keepLine")
			{
				keepLineOption.second = std::make_shared<KeepLineElement>();
				continue;
			}
			std::smatch m;

			if (std::regex_search(value, m, minLineRegex))
			{
				keepLineOption.second = std::make_shared<MinLineElement>(std::stoi(m.str(1)));
				continue;
			}

			if (std::regex_search(value, m, keepLineRegex))
			{
				keepLineOption.second = std::make_shared<KeepLineElement>(std::stoi(m.str(1)));
			}
		}
	}

	if (configMap.count("insert_final_newline"))
	{
		options->insert_final_newline = configMap.at("insert_final_newline") == "true";
	}

	if (configMap.count("enable_name_style_check"))
	{
		options->enable_name_style_check = configMap.at("enable_name_style_check") == "true";
	}
	//
	// std::vector<std::pair<std::string, NameStyle&>> styleList = {
	// 	{"local_name_define_style", options->local_name_define_style},
	// 	{"function_name_define_style", options->function_name_define_style},
	// 	{"table_field_name_define_style", options->table_field_name_define_style},
	// 	{"global_variable_name_define_style", options->table_field_name_define_style}
	// };
	//
	// for (auto& styleOption : styleList)
	// {
	// 	if (configMap.count(styleOption.first))
	// 	{
	// 		std::string value = configMap.at(styleOption.first);
	// 		if (value == "off")
	// 		{
	// 			styleOption.second = NameStyle::Off;
	// 		}
	// 		else if (value == "snake_case")
	// 		{
	// 			styleOption.second = NameStyle::SnakeCase;
	// 		}
	// 		else if (value == "camel_case")
	// 		{
	// 			styleOption.second = NameStyle::CamelCase;
	// 		}
	// 		else if (value == "pascal_case")
	// 		{
	// 			styleOption.second = NameStyle::PascalCase;
	// 		}
	// 	}
	// }
}