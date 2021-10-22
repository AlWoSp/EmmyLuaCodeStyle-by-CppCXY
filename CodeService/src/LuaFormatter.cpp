#include "CodeService/LuaFormatter.h"
#include "CodeService/FormatElement/IndentElement.h"
#include "CodeService/FormatElement/StatementElement.h"
#include "CodeService/FormatElement/TextElement.h"
#include "CodeService/FormatElement/FormatContext.h"
#include "CodeService/FormatElement/KeepLineElement.h"
#include "CodeService/FormatElement/MinLineElement.h"
#include "CodeService/FormatElement/KeepBlankElement.h"
#include "CodeService/FormatElement/ExpressionElement.h"
#include "CodeService/FormatElement/LineElement.h"
#include "CodeService/FormatElement/AlignmentLayoutElement.h"
#include "CodeService/FormatElement/AlignToFirstElement.h"
#include "CodeService/FormatElement/KeepElement.h"
#include "CodeService/FormatElement/LongExpressionLayoutElement.h"

bool nextMatch(int currentIndex, LuaAstNodeType type, const std::vector<std::shared_ptr<LuaAstNode>>& vec)
{
	if (currentIndex >= 0 && (currentIndex + 1) < vec.size())
	{
		return vec[currentIndex + 1]->GetType() == type;
	}

	return false;
}

std::shared_ptr<LuaAstNode> nextNode(int currentIndex, const std::vector<std::shared_ptr<LuaAstNode>>& vec)
{
	if (currentIndex >= 0 && (currentIndex + 1) < vec.size())
	{
		return vec[currentIndex + 1];
	}

	return nullptr;
}


LuaFormatter::LuaFormatter(std::shared_ptr<LuaParser> luaParser, LuaFormatOptions& options)
	: _parser(luaParser),
	  _options(options)
{
}

void LuaFormatter::BuildFormattedElement()
{
	auto chunkNode = _parser->GetAst();

	_env = FormatBlock(chunkNode->GetChildren()[0]);
}

std::string LuaFormatter::GetFormattedText()
{
	LuaFormatOptions options;
	FormatContext ctx(_parser, options);

	_env->Serialize(ctx, 0, nullptr);

	return ctx.GetText();
}

void LuaFormatter::AddDiagnosis(TextRange range, std::string_view message)
{
}

std::vector<LuaCodeDiagnosis>& LuaFormatter::GetDiagnosis()
{
	return _diagnosis;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatNode(std::shared_ptr<LuaAstNode> node)
{
	switch (node->GetType())
	{
	case LuaAstNodeType::Block:
		{
			return FormatBlock(node);
		}
	case LuaAstNodeType::LocalStatement:
		{
			return FormatLocalStatement(node);
		}
	case LuaAstNodeType::NameDefList:
		{
			return FormatNameDefList(node);
		}
	case LuaAstNodeType::ExpressionList:
		{
			return FormatExpressionList(node);
		}
	case LuaAstNodeType::Comment:
		{
			return FormatComment(node);
		}
	case LuaAstNodeType::LabelStatement:
		{
			return FormatLabelStatement(node);
		}
	case LuaAstNodeType::BreakStatement:
		{
			return FormatBreakStatement(node);
		}
	case LuaAstNodeType::GotoStatement:
		{
			return FormatGotoStatement(node);
		}
	case LuaAstNodeType::ReturnStatement:
		{
			return FormatReturnStatement(node);
		}
	case LuaAstNodeType::Identify:
		{
			return FormatIdentify(node);
		}
	case LuaAstNodeType::GeneralOperator:
		{
			return FormatGeneralOperator(node);
		}
	case LuaAstNodeType::KeyWord:
		{
			return FormatKeyWords(node);
		}
	case LuaAstNodeType::WhileStatement:
		{
			return FormatWhileStatement(node);
		}
	case LuaAstNodeType::ForStatement:
		{
			return FormatForStatement(node);
		}
	case LuaAstNodeType::ForBody:
		{
			return FormatForBody(node);
		}
	case LuaAstNodeType::Attribute:
		{
			return FormatAttribute(node);
		}
	case LuaAstNodeType::RepeatStatement:
		{
			return FormatRepeatStatement(node);
		}
	case LuaAstNodeType::DoStatement:
		{
			return FormatDoStatement(node);
		}
	case LuaAstNodeType::AssignStatement:
		{
			return FormatAssignment(node);
		}
	case LuaAstNodeType::IfStatement:
		{
			return FormatIfStatement(node);
		}
	case LuaAstNodeType::ExpressionStatement:
		{
			return FormatExpressionStatement(node);
		}
	case LuaAstNodeType::Expression:
		{
			return FormatExpression(node);
		}
	case LuaAstNodeType::BinaryExpression:
		{
			return FormatBinaryExpression(node);
		}
	case LuaAstNodeType::UnaryExpression:
		{
			return FormatUnaryExpression(node);
		}
	case LuaAstNodeType::PrimaryExpression:
		{
			return FormatPrimaryExpression(node);
		}
	case LuaAstNodeType::CallExpression:
		{
			return FormatCallExpression(node);
		}
	case LuaAstNodeType::CallArgList:
		{
			return FormatCallArgList(node);
		}
	case LuaAstNodeType::FunctionStatement:
		{
			return FormatFunctionStatement(node);
		}
	case LuaAstNodeType::LocalFunctionStatement:
		{
			return FormatLocalFunctionStatement(node);
		}
	case LuaAstNodeType::ClosureExpression:
		{
			return FormatClosureExpression(node);
		}
	case LuaAstNodeType::NameExpression:
		{
			return FormatNameExpression(node);
		}
	case LuaAstNodeType::TableExpression:
		{
			return FormatTableExpression(node);
		}
	case LuaAstNodeType::TableField:
		{
			return FormatTableField(node);
		}
	case LuaAstNodeType::IndexExpression:
		{
			return FormatIndexExpression(node);
		}
	case LuaAstNodeType::ParamList:
		{
			return FormatParamList(node);
		}
	case LuaAstNodeType::FunctionBody:
		{
			return FormatFunctionBody(node);
		}
	case LuaAstNodeType::LiteralExpression:
	default:
		{
			return std::make_shared<TextElement>(node->GetText(), node->GetTextRange());
		}
	}
}

std::shared_ptr<FormatElement> LuaFormatter::FormatBlock(std::shared_ptr<LuaAstNode> blockNode)
{
	auto indentEnv = std::make_shared<IndentElement>();

	auto statements = blockNode->GetChildren();

	for (int index = 0; index < statements.size(); index++)
	{
		auto statement = statements[index];
		switch (statement->GetType())
		{
		case LuaAstNodeType::AssignStatement:
		case LuaAstNodeType::LocalStatement:
			{
				if (nextMatch(index, LuaAstNodeType::AssignStatement, statements)
					|| nextMatch(index, LuaAstNodeType::LocalStatement, statements)
					|| nextMatch(index, LuaAstNodeType::Comment, statements))
				{
					indentEnv->AddChild(FormatAlignStatement(index, statements));
				}
				else
				{
					auto childEnv = FormatNode(statement);
					indentEnv->AddChild(childEnv);
				}
				indentEnv->Add<KeepLineElement>();
				break;
			}
		case LuaAstNodeType::RepeatStatement:
		case LuaAstNodeType::DoStatement:
		case LuaAstNodeType::ForStatement:
		case LuaAstNodeType::WhileStatement:
			{
				auto childEnv = FormatNode(statement);
				indentEnv->AddChild(childEnv);

				indentEnv->Add<MinLineElement>(1);
				break;
			}
		case LuaAstNodeType::Comment:
			{
				auto comment = FormatNode(statement);
				auto commentStatement = std::make_shared<StatementElement>();
				commentStatement->AddChild(comment);
				indentEnv->AddChild(commentStatement);

				indentEnv->Add<KeepLineElement>();
				break;
			}
		case LuaAstNodeType::BreakStatement:
		case LuaAstNodeType::ReturnStatement:
		case LuaAstNodeType::GotoStatement:
		case LuaAstNodeType::ExpressionStatement:
			{
				indentEnv->AddChild(FormatNode(statement));

				indentEnv->Add<KeepLineElement>();
				break;
			}
		case LuaAstNodeType::LocalFunctionStatement:
		case LuaAstNodeType::FunctionStatement:
			{
				auto childEnv = FormatNode(statement);
				indentEnv->AddChild(childEnv);

				if (nextMatch(index, LuaAstNodeType::FunctionStatement, statements))
				{
					indentEnv->Add<KeepLineElement>(1);
				}
				else
				{
					indentEnv->Add<MinLineElement>(1);
				}
				break;
			}
		default:
			{
				auto childEnv = FormatNode(statement);
				indentEnv->AddChild(childEnv);
				indentEnv->Add<KeepLineElement>();
			}
		}
	}

	return indentEnv;
}


std::shared_ptr<FormatElement> LuaFormatter::FormatLocalStatement(std::shared_ptr<LuaAstNode> localStatement)
{
	auto env = std::make_shared<StatementElement>();
	for (auto& node : localStatement->GetChildren())
	{
		switch (node->GetType())
		{
		case LuaAstNodeType::KeyWord:
		case LuaAstNodeType::GeneralOperator:
			{
				env->Add<TextElement>(node);
				env->Add<KeepBlankElement>(1);
				break;
			}
		case LuaAstNodeType::NameDefList:
			{
				env->AddChild(FormatNode(node));
				env->Add<KeepBlankElement>(1);
				break;
			}
		case LuaAstNodeType::ExpressionList:
			{
				env->AddChild(FormatNode(node));
				break;
			}
		default:
			DefaultHandle(node, env);
			break;
		}
	}

	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatAssignment(std::shared_ptr<LuaAstNode> assignStatement)
{
	auto env = std::make_shared<StatementElement>();
	bool isLeftExprList = true;
	for (auto& node : assignStatement->GetChildren())
	{
		switch (node->GetType())
		{
		case LuaAstNodeType::GeneralOperator:
			{
				env->Add<TextElement>(node);
				env->Add<KeepBlankElement>(1);
				break;
			}
		case LuaAstNodeType::ExpressionList:
			{
				env->AddChild(FormatNode(node));
				if (isLeftExprList)
				{
					env->Add<KeepBlankElement>(1);
					isLeftExprList = false;
				}
				break;
			}
		default:
			DefaultHandle(node, env);
			break;
		}
	}

	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatNameDefList(std::shared_ptr<LuaAstNode> nameDefList)
{
	auto env = std::make_shared<ExpressionElement>();

	for (auto node : nameDefList->GetChildren())
	{
		switch (node->GetType())
		{
		case LuaAstNodeType::Identify:
			{
				env->Add<TextElement>(node);
				break;
			}
		case LuaAstNodeType::GeneralOperator:
			{
				env->Add<TextElement>(node);
				env->Add<KeepBlankElement>(1);
				break;
			}
		default:
			DefaultHandle(node, env);
			break;
		}
	}

	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatExpressionList(std::shared_ptr<LuaAstNode> expressionList)
{
	auto env = std::make_shared<ExpressionElement>();

	for (auto node : expressionList->GetChildren())
	{
		switch (node->GetType())
		{
		case LuaAstNodeType::Expression:
			{
				env->AddChild(FormatNode(node));
				break;
			}
		case LuaAstNodeType::GeneralOperator:
			{
				env->Add<TextElement>(node);
				env->Add<KeepBlankElement>(1);
				break;
			}
		default:
			DefaultHandle(node, env);
		}
	}

	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatComment(std::shared_ptr<LuaAstNode> comment)
{
	auto env = std::make_shared<ExpressionElement>();
	env->Add<TextElement>(comment);

	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatBreakStatement(std::shared_ptr<LuaAstNode> breakNode)
{
	auto env = std::make_shared<StatementElement>();

	for (auto child : breakNode->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::KeyWord:
		case LuaAstNodeType::GeneralOperator:
			{
				env->Add<TextElement>(child);
				break;
			}
		default:
			DefaultHandle(child, env);
			break;
		}
	}
	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatReturnStatement(std::shared_ptr<LuaAstNode> returnNode)
{
	auto env = std::make_shared<StatementElement>();

	for (auto child : returnNode->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::KeyWord:
		case LuaAstNodeType::GeneralOperator:
			{
				env->Add<TextElement>(child);
				break;
			}
		case LuaAstNodeType::ExpressionList:
			{
				env->Add<KeepBlankElement>(1);
				env->AddChild(FormatNode(child));
				break;
			}

		default:
			DefaultHandle(child, env);
			break;
		}
	}
	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatGotoStatement(std::shared_ptr<LuaAstNode> gotoNode)
{
	auto env = std::make_shared<StatementElement>();

	for (auto child : gotoNode->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::KeyWord:
		case LuaAstNodeType::GeneralOperator:
			{
				env->Add<TextElement>(child);
				break;
			}
		case LuaAstNodeType::Identify:
			{
				env->Add<KeepBlankElement>(1);
				env->AddChild(FormatNode(child));
				break;
			}

		default:
			DefaultHandle(child, env);
			break;
		}
	}
	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatLabelStatement(std::shared_ptr<LuaAstNode> labelStatement)
{
	auto env = std::make_shared<StatementElement>();

	for (auto child : labelStatement->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::GeneralOperator:
			{
				env->Add<TextElement>(child);
				break;
			}
		case LuaAstNodeType::Identify:
			{
				env->AddChild(FormatNode(child));
				break;
			}

		default:
			DefaultHandle(child, env);
			break;
		}
	}
	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatIdentify(std::shared_ptr<LuaAstNode> identify)
{
	return std::make_shared<TextElement>(identify);
}

std::shared_ptr<FormatElement> LuaFormatter::FormatGeneralOperator(std::shared_ptr<LuaAstNode> general)
{
	return std::make_shared<TextElement>(general);
}

std::shared_ptr<FormatElement> LuaFormatter::FormatKeyWords(std::shared_ptr<LuaAstNode> keywords)
{
	return std::make_shared<TextElement>(keywords);
}

std::shared_ptr<FormatElement> LuaFormatter::FormatDoStatement(std::shared_ptr<LuaAstNode> doStatement)
{
	auto env = std::make_shared<StatementElement>();

	for (auto child : doStatement->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::KeyWord:
			{
				env->Add<TextElement>(child);
				if (child->GetText() == "do")
				{
					env->Add<LineElement>();
				}
				break;
			}
		case LuaAstNodeType::Block:
			{
				env->AddChild(FormatBlock(child));
				env->Add<KeepLineElement>();
				break;
			}
		default:
			{
				DefaultHandle(child, env);
				break;
			}
		}
	}

	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatWhileStatement(std::shared_ptr<LuaAstNode> whileStatement)
{
	auto env = std::make_shared<StatementElement>();
	for (auto child : whileStatement->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::KeyWord:
			{
				env->Add<TextElement>(child);
				if (child->GetText() == "do")
				{
					env->Add<LineElement>();
				}
				break;
			}
		case LuaAstNodeType::Expression:
			{
				auto expression = FormatNode(child);
				env->AddChild(expression);
				env->Add<KeepBlankElement>(1);
				break;
			}
		case LuaAstNodeType::Block:
			{
				env->AddChild(FormatNode(child));
				env->Add<KeepLineElement>();
				break;
			}
		default:
			{
				DefaultHandle(child, env);
				break;
			}
		}
	}
	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatForStatement(std::shared_ptr<LuaAstNode> forStatement)
{
	auto env = std::make_shared<StatementElement>();
	for (auto child : forStatement->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::KeyWord:
			{
				env->Add<TextElement>(child);
				env->Add<KeepBlankElement>(1);
				break;
			}
		case LuaAstNodeType::ForNumber:
			{
				for (auto forNumberChild : child->GetChildren())
				{
					switch (forNumberChild->GetType())
					{
					case LuaAstNodeType::Identify:
					case LuaAstNodeType::GeneralOperator:
						{
							env->Add<TextElement>(forNumberChild);
							env->Add<KeepBlankElement>(1);
							break;
						}
					case LuaAstNodeType::Expression:
						{
							env->AddChild(FormatNode(forNumberChild));
							break;
						}
					case LuaAstNodeType::ForBody:
						{
							env->Add<KeepBlankElement>(1);
							env->AddChild(FormatNode(forNumberChild));
							break;
						}
					default:
						{
							DefaultHandle(forNumberChild, env);
							break;
						}
					}
				}
				break;
			}
		case LuaAstNodeType::ForList:
			{
				for (auto forListChild : child->GetChildren())
				{
					switch (forListChild->GetType())
					{
					case LuaAstNodeType::KeyWord:
					case LuaAstNodeType::NameDefList:
					case LuaAstNodeType::ExpressionList:
						{
							env->AddChild(FormatNode(forListChild));
							env->Add<KeepBlankElement>(1);
							break;
						}
					default:
						{
							DefaultHandle(forListChild, env);
							break;
						}
					}
				}
				break;
			}
		default:
			{
				DefaultHandle(child, env);
				break;
			}
		}
	}
	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatForBody(std::shared_ptr<LuaAstNode> forBody)
{
	auto env = std::make_shared<ExpressionElement>();
	for (auto child : forBody->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::KeyWord:
			{
				env->Add<TextElement>(child);
				if (child->GetText() == "do")
				{
					env->Add<LineElement>();
				}
				break;
			}
		case LuaAstNodeType::Block:
			{
				env->AddChild(FormatNode(child));
				env->Add<KeepLineElement>();
				break;
			}
		default:
			{
				DefaultHandle(child, env);
				break;
			}
		}
	}

	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatAttribute(std::shared_ptr<LuaAstNode> attribute)
{
	auto env = std::make_shared<ExpressionElement>();

	for (auto child : attribute->GetChildren())
	{
		DefaultHandle(child, env);
	}

	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatRepeatStatement(std::shared_ptr<LuaAstNode> repeatStatement)
{
	auto env = std::make_shared<StatementElement>();

	for (auto child : repeatStatement->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::KeyWord:
			{
				env->Add<TextElement>(child);
				if (child->GetText() == "repeat")
				{
					env->Add<LineElement>();
				}
				break;
			}
		case LuaAstNodeType::Block:
			{
				env->AddChild(FormatNode(child));
				env->Add<KeepLineElement>();
				break;
			}
		default:
			{
				DefaultHandle(child, env);
				break;
			}
		}
	}

	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatIfStatement(std::shared_ptr<LuaAstNode> ifStatement)
{
	auto env = std::make_shared<StatementElement>();

	for (auto child : ifStatement->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::KeyWord:
			{
				env->Add<TextElement>(child);
				if (child->GetText() == "then" || child->GetText() == "else")
				{
					env->Add<LineElement>();
				}
				else if (child->GetText() == "if" || child->GetText() == "elseif")
				{
					env->Add<KeepBlankElement>(1);
				}
				break;
			}
		case LuaAstNodeType::Expression:
			{
				env->AddChild(FormatNode(child));
				env->Add<KeepBlankElement>(1);
				break;
			}
		case LuaAstNodeType::Block:
			{
				env->AddChild(FormatNode(child));
				env->Add<KeepLineElement>();
				break;
			}
		default:
			{
				DefaultHandle(child, env);
				break;
			}
		}
	}

	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatExpressionStatement(std::shared_ptr<LuaAstNode> expressionStatement)
{
	auto env = std::make_shared<StatementElement>();

	for (auto child : expressionStatement->GetChildren())
	{
		switch (child->GetType())
		{
			// Ŀǰ���ʽ�����ֻ�е��ñ��ʽ
		case LuaAstNodeType::CallExpression:
			{
				env->AddChild(FormatNode(child));
				break;
			}
			// default һ��ֻ��һ���ֺ�
		default:
			{
				DefaultHandle(child, env);
			}
		}
	}

	return env;
}

/*
 * ���ʽ������кܴ�ĸ�����
 * ������ʽ�����ˣ������������Ǻܸ��ӵ�����
 */
std::shared_ptr<FormatElement> LuaFormatter::FormatExpression(std::shared_ptr<LuaAstNode> expression)
{
	auto env = std::make_shared<ExpressionElement>();

	auto& children = expression->GetChildren();
	for (int i = 0; i != children.size(); i++)
	{
		auto current = children[i];

		env->AddChild(FormatNode(current));

		auto next = nextNode(i, children);

		if (next)
		{
			auto nextLine = _parser->GetLine(next->GetTextRange().StartOffset);
			auto currentLine = _parser->GetLine(current->GetTextRange().EndOffset);

			if (currentLine == nextLine)
			{
				env->Add<KeepBlankElement>(1);
			}
			else
			{
				env->Add<LineElement>();
				env->Add<KeepLineElement>();
			}
		}
	}

	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatBinaryExpression(std::shared_ptr<LuaAstNode> binaryExpression)
{
	return FormatExpression(binaryExpression);
}

std::shared_ptr<FormatElement> LuaFormatter::FormatUnaryExpression(std::shared_ptr<LuaAstNode> unaryExpression)
{
	auto env = std::make_shared<ExpressionElement>();

	auto& children = unaryExpression->GetChildren();
	for (int i = 0; i != children.size(); i++)
	{
		auto current = children[i];

		env->AddChild(FormatNode(current));
		auto next = nextNode(i, children);

		if (next)
		{
			auto nextLine = _parser->GetLine(next->GetTextRange().StartOffset);
			auto currentLine = _parser->GetLine(current->GetTextRange().EndOffset);

			if (currentLine == nextLine)
			{
				if (current->GetText() != "not")
				{
					env->Add<KeepBlankElement>(1);
				}
			}
			else
			{
				env->Add<LineElement>();
				env->Add<KeepLineElement>();
			}
		}
	}

	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatCallExpression(std::shared_ptr<LuaAstNode> callExpression)
{
	auto env = std::make_shared<ExpressionElement>();
	for (auto child : callExpression->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::PrimaryExpression:
			{
				env->AddChild(FormatNode(child));
				break;
			}
		case LuaAstNodeType::CallArgList:
			{
				env->AddChild(FormatNode(child));
				break;
			}
		default:
			{
				DefaultHandle(child, env);
			}
		}
	}
	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatCallArgList(std::shared_ptr<LuaAstNode> callArgList)
{
	auto env = std::make_shared<ExpressionElement>();

	for (auto child : callArgList->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::GeneralOperator:
			{
				env->Add<TextElement>(child);
				if (child->GetText() == ",")
				{
					env->Add<KeepBlankElement>(1);
				}
				break;
			}
		case LuaAstNodeType::Expression:
		case LuaAstNodeType::Identify:
			{
				env->AddChild(FormatNode(child));
				break;
			}
		default:
			{
				DefaultHandle(child, env);
			}
		}
	}
	return env;
}


std::shared_ptr<FormatElement> LuaFormatter::FormatPrimaryExpression(std::shared_ptr<LuaAstNode> primaryExpression)
{
	auto env = std::make_shared<ExpressionElement>();

	for (auto child : primaryExpression->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::GeneralOperator:
			{
				env->Add<TextElement>(child);
				break;
			}
		case LuaAstNodeType::Expression:
		case LuaAstNodeType::Identify:
			{
				env->AddChild(FormatNode(child));
				break;
			}
		default:
			{
				DefaultHandle(child, env);
			}
		}
	}
	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatFunctionStatement(std::shared_ptr<LuaAstNode> functionStatement)
{
	auto env = std::make_shared<StatementElement>();

	for (auto child : functionStatement->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::KeyWord:
			{
				env->Add<TextElement>(child);
				env->Add<KeepBlankElement>(1);
				break;
			}
		case LuaAstNodeType::NameExpression:
			{
				env->AddChild(FormatNode(child));
				break;
			}
		case LuaAstNodeType::FunctionBody:
			{
				env->AddChild(FormatNode(child));
				break;
			}
		default:
			{
				DefaultHandle(child, env);
			}
		}
	}
	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatNameExpression(std::shared_ptr<LuaAstNode> nameExpression)
{
	auto env = std::make_shared<ExpressionElement>();

	for (auto child : nameExpression->GetChildren())
	{
		DefaultHandle(child, env);
	}
	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatParamList(std::shared_ptr<LuaAstNode> paramList)
{
	auto env = std::make_shared<ExpressionElement>();
	auto& children = paramList->GetChildren();
	auto alignToFirst = std::make_shared<AlignToFirstElement>(_options.Indent);
	for (int i = 0; i != children.size(); i++)
	{
		auto child = children[i];
		switch (child->GetType())
		{
		case LuaAstNodeType::GeneralOperator:
			{
				if (child->GetText() == ",")
				{
					alignToFirst->Add<TextElement>(child);
					alignToFirst->Add<KeepElement>(1);
				}
				else if (child->GetText() == ")")
				{
					alignToFirst->Add<TextElement>(child);
					env->AddChild(alignToFirst);
				}
				else
				{
					env->Add<TextElement>(child);
					env->Add<KeepElement>(0);
				}
				break;
			}
		case LuaAstNodeType::Param:
			{
				alignToFirst->Add<TextElement>(child);
				if (nextMatch(i, LuaAstNodeType::Comment, children))
				{
					alignToFirst->Add<KeepElement>(1);
				}
				else
				{
					alignToFirst->Add<KeepElement>(0);
				}

				break;
			}
		case LuaAstNodeType::Comment:
			{
				alignToFirst->Add<TextElement>(child);
				alignToFirst->Add<KeepElement>(1);
				break;
			}
		default:
			{
				DefaultHandle(child, env);
			}
		}
	}


	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatFunctionBody(std::shared_ptr<LuaAstNode> functionBody)
{
	auto env = std::make_shared<ExpressionElement>();

	auto& children = functionBody->GetChildren();

	for (int i = 0; i < children.size(); i++)
	{
		auto child = children[i];
		switch (child->GetType())
		{
		case LuaAstNodeType::ParamList:
			{
				env->AddChild(FormatNode(child));

				auto nextChild = nextNode(i, children);
				//����һ������������ functionBody ֻ��һ�д��룬�����ͼ���ǲ�����
				if(nextChild && nextChild->GetType() == LuaAstNodeType::Block)
				{
					auto range = functionBody->GetTextRange();
					if (_parser->GetLine(range.StartOffset) == _parser->GetLine(range.EndOffset))
					{
						env->Add<KeepElement>(1);
						auto block = FormatNode(nextChild);
						for(auto blockChild : block->GetChildren())
						{
							auto shortExpression = std::make_shared<ExpressionElement>();
							shortExpression->AddChildren(blockChild->GetChildren());
							env->AddChild(shortExpression);
							env->Add<KeepElement>(1);
						}
						i++;
					}
					else
					{
						env->Add<LineElement>();
					}
				}
				else
				{
					env->Add<KeepElement>(1);
				}
				break;
			}
		case LuaAstNodeType::Comment:
			{
				env->AddChild(FormatNode(child));
				env->Add<KeepElement>(1);
				break;
			}
			
		case LuaAstNodeType::Block:
			{
				env->AddChild(FormatNode(child));
				env->Add<KeepLineElement>();

				break;
			}
		case LuaAstNodeType::KeyWord:
			{
				env->Add<TextElement>(child);
				env->Add<KeepElement>(0);
				break;
			}
		default:
			{
				DefaultHandle(child, env);
			}
		}
	}
	return env;
}


std::shared_ptr<FormatElement> LuaFormatter::FormatClosureExpression(std::shared_ptr<LuaAstNode> closureExpression)
{
	auto env = std::make_shared<ExpressionElement>();

	for (auto child : closureExpression->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::KeyWord:
			{
				env->Add<TextElement>(child);
				break;
			}
		case LuaAstNodeType::FunctionBody:
			{
				env->AddChild(FormatNode(child));
				break;
			}
		default:
			{
				DefaultHandle(child, env);
			}
		}
	}
	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatLocalFunctionStatement(
	std::shared_ptr<LuaAstNode> localFunctionStatement)
{
	auto env = std::make_shared<StatementElement>();

	for (auto child : localFunctionStatement->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::KeyWord:
			{
				env->Add<TextElement>(child);
				env->Add<KeepBlankElement>(1);
				break;
			}
		case LuaAstNodeType::Identify:
			{
				env->AddChild(FormatNode(child));
				break;
			}
		case LuaAstNodeType::FunctionBody:
			{
				env->AddChild(FormatNode(child));
				break;
			}
		default:
			{
				DefaultHandle(child, env);
			}
		}
	}
	return env;
}

/*
 * ����ʽ�м��ֳ������Ű���Ϊ
 * print {
 *	aaa,bbb,ddd,eee
 *  ffff = 123,
 *	aaaaa = 456,
 *	['agfwg'] = 123
 * }
 * 
 * local t = { a,b,c,d
 *			   e,f,g,i }
 *
 * call {
 * aa   = 1,
 * bbbb = 2,
 * dddd = 3,
 * ff   = 4,
 * ee   = 5,
 * ffff = 7
 * }
 *
 *
 */
std::shared_ptr<FormatElement> LuaFormatter::FormatTableExpression(std::shared_ptr<LuaAstNode> tableExpression)
{
	auto env = std::make_shared<ExpressionElement>();
	auto& children = tableExpression->GetChildren();
	for (int index = 0; index != children.size(); index++)
	{
		auto child = children[index];
		switch (child->GetType())
		{
		case LuaAstNodeType::GeneralOperator:
			{
				if (child->GetText() == "{")
				{
					env->Add<TextElement>(child);
					auto next = nextNode(index, children);
					if (next)
					{
						if (next->GetType() == LuaAstNodeType::GeneralOperator)
						{
							break;
						}

						if (_parser->GetLine(next->GetTextRange().StartOffset) == _parser->GetLine(
							child->GetTextRange().EndOffset))
						{
							env->Add<KeepBlankElement>(1);
						}
						else
						{
							env->Add<LineElement>();
							env->Add<KeepLineElement>();
						}
						index++;
						env->AddChild(FormatAlignTableField(index, children));
					}
				}
				else if (child->GetText() == "}")
				{
					if (index != 0)
					{
						auto lastChild = children[index - 1];
						if (_parser->GetLine(lastChild->GetTextRange().EndOffset) == _parser->GetLine(
							child->GetTextRange().StartOffset))
						{
							env->Add<KeepBlankElement>(1);
							env->Add<TextElement>(child);
						}
						else
						{
							env->Add<LineElement>();
							env->Add<KeepLineElement>();
							env->Add<TextElement>(child);
						}
					}
				}

				break;
			}
		default:
			{
				DefaultHandle(child, env);
			}
		}
	}
	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatTableField(std::shared_ptr<LuaAstNode> tableField)
{
	auto env = std::make_shared<ExpressionElement>();

	for (auto child : tableField->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::GeneralOperator:
			{
				if (child->GetText() == "=")
				{
					env->Add<KeepBlankElement>(1);
					env->Add<TextElement>(child);
					env->Add<KeepBlankElement>(1);
				}
				else
				{
					env->Add<TextElement>(child);
				}
				break;
			}
		default:
			{
				DefaultHandle(child, env);
			}
		}
	}
	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatIndexExpression(std::shared_ptr<LuaAstNode> indexExpression)
{
	auto env = std::make_shared<ExpressionElement>();

	for (auto child : indexExpression->GetChildren())
	{
		switch (child->GetType())
		{
		case LuaAstNodeType::IndexOperator:
			{
				env->AddChild(FormatNode(child));
				break;
			}
		default:
			{
				DefaultHandle(child, env);
			}
		}
	}
	return env;
}


void LuaFormatter::DefaultHandle(std::shared_ptr<LuaAstNode> node, std::shared_ptr<FormatElement> envElement)
{
	auto childEnv = FormatNode(node);
	envElement->AddChild(childEnv);
}

std::shared_ptr<FormatElement> LuaFormatter::FormatAlignStatement(int& currentIndex,
                                                                  const std::vector<std::shared_ptr<LuaAstNode>>& vec)
{
	auto env = std::make_shared<AlignmentLayoutElement>();

	env->AddChild(FormatNode(vec[currentIndex]));

	if (currentIndex + 1 >= vec.size())
	{
		return env->GetChildren()[0];
	}
	auto nextChild = vec[currentIndex + 1];

	while (nextChild->GetType() == LuaAstNodeType::AssignStatement
		|| nextChild->GetType() == LuaAstNodeType::LocalStatement
		|| nextChild->GetType() == LuaAstNodeType::Comment)
	{
		auto currentChild = vec[currentIndex];
		int currentLine = _parser->GetLine(currentChild->GetTextRange().EndOffset);
		int nextLine = _parser->GetLine(nextChild->GetTextRange().StartOffset);
		// �����������һ�������ĸ�ֵ/local/ע������������һ����ֵ/local/ע����� ���2�����ϣ�����Ϊ������
		if (nextLine - currentLine > 2)
		{
			break;
		}

		// ����Ƿ��������ע�� ���� local t = 123 -- inline comment
		if ((currentChild->GetType() == LuaAstNodeType::LocalStatement || currentChild->GetType() ==
				LuaAstNodeType::AssignStatement) && nextChild->GetType() == LuaAstNodeType::Comment
			&& nextLine == currentLine)
		{
			auto lastStatementEnv = env->LastValidElement();
			if (lastStatementEnv)
			{
				lastStatementEnv->Add<KeepBlankElement>(1);
				lastStatementEnv->Add<TextElement>(nextChild);
			}
			//else Ӧ�ò������������
		}
		else
		{
			env->AddChild(FormatNode(nextChild));
			env->Add<KeepLineElement>();
		}

		currentIndex++;

		if ((currentIndex + 1) >= vec.size())
		{
			break;
		}

		nextChild = vec[currentIndex + 1];
	}
	// ������Ǻ���������������򷵻ر���
	if (env->GetChildren().size() == 1)
	{
		return env->GetChildren()[0];
	}

	return env;
}

std::shared_ptr<FormatElement> LuaFormatter::FormatAlignTableField(int& currentIndex,
                                                                   const std::vector<std::shared_ptr<LuaAstNode>>& vec)
{
	auto env = std::make_shared<AlignToFirstElement>(_options.Indent);

	env->AddChild(FormatNode(vec[currentIndex]));

	if (currentIndex + 1 >= vec.size())
	{
		return env;
	}
	auto nextChild = vec[currentIndex + 1];
	bool alignToEq = true;
	while (nextChild->GetType() == LuaAstNodeType::TableField
		|| nextChild->GetType() == LuaAstNodeType::TableFieldSep
		|| nextChild->GetType() == LuaAstNodeType::Comment)
	{
		auto currentChild = vec[currentIndex];
		int currentLine = _parser->GetLine(currentChild->GetTextRange().EndOffset);
		int nextLine = _parser->GetLine(nextChild->GetTextRange().StartOffset);
		if (nextLine == currentLine)
		{
			// ����Ƿ��������ע��
			// ���� t = 123, -- inline comment
			// ���� c = 456 --fff
			// ���� ddd = 123 --[[ffff]] ,
			if ((currentChild->GetType() == LuaAstNodeType::TableField || currentChild->GetType() ==
				LuaAstNodeType::TableFieldSep) && nextChild->GetType() == LuaAstNodeType::Comment)
			{
				env->Add<KeepBlankElement>(1);
				env->Add<TextElement>(nextChild);
			}
			else if ((currentChild->GetType() == LuaAstNodeType::TableFieldSep) && nextChild->GetType() ==
				LuaAstNodeType::TableField)
			{
				// ��ʱ��Ϊtable ��Ӧ�ÿ��Ƕ��뵽�Ⱥ�
				alignToEq = false;
				env->Add<KeepBlankElement>(1);
				env->AddChild(FormatNode(nextChild));
			}
			else if ((currentChild->GetType() == LuaAstNodeType::TableField) && nextChild->GetType() ==
				LuaAstNodeType::TableFieldSep)
			{
				env->AddChild(FormatNode(nextChild));
			}
			else
			{
				env->Add<KeepBlankElement>(1);
				env->AddChild(FormatNode(nextChild));
			}
		}
		else
		{
			env->Add<LineElement>();
			env->Add<KeepLineElement>();
			env->AddChild(FormatNode(nextChild));
		}

		currentIndex++;

		if ((currentIndex + 1) >= vec.size())
		{
			break;
		}

		nextChild = vec[currentIndex + 1];
	}

	// ��ΪtableField ����(�����Ǳ���������)���յȺŶ���
	if (alignToEq)
	{
		auto newEnv = std::make_shared<AlignToFirstElement>(_options.Indent);
		auto alignmentLayoutElement = std::make_shared<AlignmentLayoutElement>();
		for (auto child : env->GetChildren())
		{
			alignmentLayoutElement->AddChild(child);
		}

		newEnv->AddChild(alignmentLayoutElement);

		env = newEnv;
	}

	return env;
}
