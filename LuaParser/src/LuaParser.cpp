#include "LuaParser/LuaParser.h"
#include <fstream>
#include <sstream>
#include "LuaDefine.h"
#include "LuaTokenTypeDetail.h"
#include "LuaParser/LuaOperatorType.h"
#include "Util/format.h"

std::shared_ptr<LuaParser> LuaParser::LoadFromFile(std::string_view filename)
{
	std::fstream fin(std::string(filename), std::ios::in);

	if (fin.is_open())
	{
		std::stringstream s;
		s << fin.rdbuf();

		return LoadFromBuffer(s.str());
	}

	return nullptr;
}

std::shared_ptr<LuaParser> LuaParser::LoadFromBuffer(std::string&& buffer)
{
	auto tokenParser = std::make_shared<LuaTokenParser>(std::move(buffer));
	tokenParser->Parse();

	return std::make_shared<LuaParser>(tokenParser);
}

void LuaParser::BuildAst()
{
	_chunkAstNode = createAstNode(LuaAstNodeType::Chunk);

	block(_chunkAstNode);

	auto tokenErrors = _tokenParser->GetErrors();

	for (auto& tokeError : tokenErrors)
	{
		_errors.push_back(tokeError);
	}
}

std::shared_ptr<LuaAstNode> LuaParser::GetAst()
{
	return _chunkAstNode;
}

std::vector<LuaError>& LuaParser::GetErrors()
{
	return _errors;
}

bool LuaParser::HasError() const
{
	return !_errors.empty();
}

void LuaParser::BuildAstWithComment()
{
	BuildAst();

	auto& chunkChildren = _chunkAstNode->GetChildren();
	if (chunkChildren.empty())
	{
		return;
	}

	auto blockNode = chunkChildren[0];

	auto& comments = _tokenParser->GetComments();
	// ��ע��ע��AST��
	if (!comments.empty())
	{
		for (auto& comment : comments)
		{
			std::shared_ptr<LuaAstNode> commentAst = nullptr;

			if (comment.TokenType == TK_SHORT_COMMENT)
			{
				commentAst = createAstNodeFromToken(LuaAstNodeType::ShortComment, comment);
			}
			else if (comment.TokenType == TK_LONG_COMMENT)
			{
				commentAst = createAstNodeFromToken(LuaAstNodeType::LongComment, comment);
			}
			else
			{
				commentAst = createAstNodeFromToken(LuaAstNodeType::ShebangComment, comment);
			}

			blockNode->AddLeafChild(commentAst);
		}
	}
}

LuaParser::LuaParser(std::shared_ptr<LuaTokenParser> tokenParser)
	: _tokenParser(tokenParser),
	  _chunkAstNode(nullptr)
{
}

bool LuaParser::blockFollow(bool withUntil)
{
	switch (_tokenParser->Current().TokenType)
	{
	case TK_ELSE:
	case TK_ELSEIF:
	case TK_END:
	case TK_EOS:
		return true;
	case TK_UNTIL:
		return withUntil;
	default: return false;
	}
}

void LuaParser::statementList(std::shared_ptr<LuaAstNode> blockNode)
{
	while (!blockFollow(true))
	{
		statement(blockNode);
	}
}

void LuaParser::statement(std::shared_ptr<LuaAstNode> blockNode)
{
	switch (_tokenParser->Current().TokenType)
	{
	case ';':
		{
			auto emptyStatement = createAstNodeFromToken(LuaAstNodeType::EmptyStatement, _tokenParser->Current());
			blockNode->AddChild(emptyStatement);
			_tokenParser->Next();
			break;
		}
	case TK_IF:
		{
			ifStatement(blockNode);
			break;
		}
	case TK_WHILE:
		{
			whileStatement(blockNode);
			break;
		}
	case TK_DO:
		{
			doStatement(blockNode);
			break;
		}
	case TK_FOR:
		{
			forStatement(blockNode);
			break;
		}
	case TK_REPEAT:
		{
			repeatStatement(blockNode);
			break;
		}
	case TK_FUNCTION:
		{
			functionStatement(blockNode);
			break;
		}
	case TK_LOCAL:
		{
			if (_tokenParser->LookAhead().TokenType == TK_FUNCTION)
			{
				localFunctionStatement(blockNode);
			}
			else
			{
				localStatement(blockNode);
			}
			break;
		}
	case TK_DBCOLON:
		{
			LabelStatement(blockNode);
			break;
		}
	case TK_RETURN:
		{
			returnStatement(blockNode);
			break;
		}
	case TK_BREAK:
		{
			breakStatement(blockNode);
			break;
		}
	case TK_GOTO:
		{
			gotoStatement(blockNode);
			break;
		}
	default:
		{
			expressionStatement(blockNode);
			break;
		}
	}
}

/* ifstat -> IF cond THEN block {ELSEIF cond THEN block} [ELSE block] END */
void LuaParser::ifStatement(std::shared_ptr<LuaAstNode> blockNode)
{
	auto ifNode = createAstNode(LuaAstNodeType::IfStatement);

	testThenBlock(ifNode);
	while (_tokenParser->Current().TokenType == TK_ELSEIF)
	{
		testThenBlock(ifNode);
	}
	if (testNext(TK_ELSE, ifNode))
	{
		block(ifNode);
	}
	checkMatch(TK_END, TK_IF, ifNode);

	blockNode->AddChild(ifNode);
}

/* whilestat -> WHILE cond DO block END */
void LuaParser::whileStatement(std::shared_ptr<LuaAstNode> blockNode)
{
	auto whileStatement = createAstNode(LuaAstNodeType::WhileStatement);

	checkAndNext(TK_WHILE, whileStatement);

	condition(whileStatement);

	checkAndNext(TK_DO, whileStatement);

	block(whileStatement);

	checkMatch(TK_END, TK_WHILE, whileStatement);

	blockNode->AddChild(whileStatement);
}

void LuaParser::doStatement(std::shared_ptr<LuaAstNode> blockNode)
{
	auto doStatement = createAstNode(LuaAstNodeType::DoStatement);

	checkAndNext(TK_DO, doStatement);

	block(doStatement);

	checkMatch(TK_END, TK_DO, doStatement);

	blockNode->AddChild(doStatement);
}

/* forstat -> FOR (fornum | forlist) END */
void LuaParser::forStatement(std::shared_ptr<LuaAstNode> blockNode)
{
	// forstatement ֻ��һ�� for ��token �ڵ� ���� forNumber����forList �ڵ�
	auto forStatement = createAstNode(LuaAstNodeType::ForStatement);

	checkAndNext(TK_FOR, forStatement);

	check(TK_NAME);

	switch (_tokenParser->LookAhead().TokenType)
	{
	case '=':
		{
			forNumber(forStatement);
			break;
		}
	case ',':
	case TK_IN:
		{
			forList(forStatement);
			break;
		}
	default:
		{
			luaError("'=' or 'in' expected", forStatement);
		}
	}

	blockNode->AddChild(forStatement);
}

/* repeatstat -> REPEAT block UNTIL cond */
void LuaParser::repeatStatement(std::shared_ptr<LuaAstNode> blockNode)
{
	auto repeatStatement = createAstNode(LuaAstNodeType::RepeatStatement);

	checkAndNext(TK_REPEAT, repeatStatement);

	block(repeatStatement);

	checkMatch(TK_UNTIL, TK_REPEAT, repeatStatement);

	condition(repeatStatement);

	blockNode->AddChild(repeatStatement);
}

void LuaParser::functionStatement(std::shared_ptr<LuaAstNode> blockNode)
{
	auto functionStatement = createAstNode(LuaAstNodeType::FunctionStatement);

	checkAndNext(TK_FUNCTION, functionStatement);

	functionName(functionStatement);

	functionBody(functionStatement);

	blockNode->AddChild(functionStatement);
}

void LuaParser::localFunctionStatement(std::shared_ptr<LuaAstNode> blockNode)
{
	auto localFunctionStatement = createAstNode(LuaAstNodeType::LocalFunctionStatement);

	checkAndNext(TK_LOCAL, localFunctionStatement);

	checkAndNext(TK_FUNCTION, localFunctionStatement);

	checkName(localFunctionStatement);

	functionBody(localFunctionStatement);

	blockNode->AddChild(localFunctionStatement);
}

/* stat -> LOCAL ATTRIB NAME {',' ATTRIB NAME} ['=' explist] */
void LuaParser::localStatement(std::shared_ptr<LuaAstNode> blockNode)
{
	auto localStatement = createAstNode(LuaAstNodeType::LocalStatement);

	checkAndNext(TK_LOCAL, localStatement);

	auto nameDefList = createAstNode(LuaAstNodeType::NameDefList);

	do
	{
		checkName(nameDefList);
		getLocalAttribute(nameDefList);
	}
	while (testNext(',', nameDefList, LuaAstNodeType::GeneralOperator));

	localStatement->AddChild(nameDefList);

	if (testNext('=', localStatement, LuaAstNodeType::GeneralOperator))
	{
		expressionList(localStatement);
	}

	blockNode->AddChild(localStatement);
}

void LuaParser::LabelStatement(std::shared_ptr<LuaAstNode> blockNode)
{
	auto labelStatement = createAstNode(LuaAstNodeType::LabelStatement);

	checkAndNext(TK_DBCOLON, labelStatement);

	checkName(labelStatement);

	checkAndNext(TK_DBCOLON, labelStatement);

	//��Ϊlabel�ǵ��������
	blockNode->AddChild(labelStatement);
}

void LuaParser::returnStatement(std::shared_ptr<LuaAstNode> blockNode)
{
	auto returnStatement = createAstNode(LuaAstNodeType::ReturnStatement);

	checkAndNext(TK_RETURN, returnStatement);

	if (!(blockFollow(true) || _tokenParser->Current().TokenType == ';'))
	{
		expressionList(returnStatement);
	}

	testNext(';', returnStatement, LuaAstNodeType::GeneralOperator);

	blockNode->AddChild(returnStatement);
}


void LuaParser::breakStatement(std::shared_ptr<LuaAstNode> blockNode)
{
	auto breakStatement = createAstNode(LuaAstNodeType::BreakStatement);

	checkAndNext(TK_BREAK, breakStatement);
	blockNode->AddChild(breakStatement);
}

void LuaParser::gotoStatement(std::shared_ptr<LuaAstNode> blockNode)
{
	auto gotoStatement = createAstNode(LuaAstNodeType::GotoStatement);

	checkAndNext(TK_GOTO, gotoStatement);

	checkName(gotoStatement);

	blockNode->AddChild(gotoStatement);
}

/* stat -> func | assignment */
void LuaParser::expressionStatement(std::shared_ptr<LuaAstNode> blockNode)
{
	auto expressionStatement = createAstNode(LuaAstNodeType::ExpressionStatement);
	suffixedExpression(expressionStatement);
	if (_tokenParser->Current().TokenType == '=' || _tokenParser->Current().TokenType == ',')
	{
		expressionStatement->SetType(LuaAstNodeType::AssignStatement);
		//��ֵ���ʽ
		assignStatement(expressionStatement);

		blockNode->AddChild(expressionStatement);
	}
	else // call expression
	{
		blockNode->AddChild(expressionStatement);
	}
}

/*
** Parse and compile a multiple assignment. The first "variable"
** (a 'suffixedexp') was already read by the caller.
**
** assignment -> suffixedexp restassign
** restassign -> ',' suffixedexp restassign | '=' explist
*/
void LuaParser::assignStatement(std::shared_ptr<LuaAstNode> assignStatementNode)
{
	if (testNext(',', assignStatementNode, LuaAstNodeType::GeneralOperator))
	{
		suffixedExpression(assignStatementNode);

		assignStatement(assignStatementNode);
	}
	else
	{
		checkAndNext('=', assignStatementNode, LuaAstNodeType::GeneralOperator);
		expressionList(assignStatementNode);
	}
}


void LuaParser::condition(std::shared_ptr<LuaAstNode> parent)
{
	expression(parent);
}


/* test_then_block -> [IF | ELSEIF] cond THEN block */
void LuaParser::testThenBlock(std::shared_ptr<LuaAstNode> ifNode)
{
	auto ifOrElseifKeyNode = createAstNodeFromToken(LuaAstNodeType::KeyWord, _tokenParser->Current());
	ifNode->AddChild(ifOrElseifKeyNode);

	_tokenParser->Next(); /*skip if or elseif*/
	expression(ifNode);
	checkAndNext(TK_THEN, ifNode);
	block(ifNode);
}


void LuaParser::block(std::shared_ptr<LuaAstNode> parent)
{
	auto blockNode = createAstNode(LuaAstNodeType::Block);

	statementList(blockNode);

	parent->AddChild(blockNode);
}

void LuaParser::checkMatch(LuaTokenType what, LuaTokenType who, std::shared_ptr<LuaAstNode> parent, LuaAstNodeType addType)
{
	if (!testNext(what, parent, addType))
	{
		auto range = parent->GetTextRange();
		luaMatchError(format("token {} expected ,(to close {} at", what, who),
		              TextRange(range.EndOffset, range.EndOffset));
	}
}

/* explist -> expr { ',' expr } */
void LuaParser::expressionList(std::shared_ptr<LuaAstNode> parent)
{
	auto expressionListNode = createAstNode(LuaAstNodeType::ExpressionList);
	expression(expressionListNode);
	while (testNext(',', expressionListNode, LuaAstNodeType::GeneralOperator))
	{
		expression(expressionListNode);
	}

	parent->AddChild(expressionListNode);
}

void LuaParser::expression(std::shared_ptr<LuaAstNode> parent)
{
	// ���о�����ʽ�����ڱ��ʽ�ڵ���
	// ���� ��һԪ���ʽ ��Ϊ ���ʽ�ڵ��һ��child
	// ���ӱ��ʽ�Ķ���Ҳ��Ϊ���ʽ�ڵ��child
	auto expressionNode = createAstNode(LuaAstNodeType::Expression);
	subexpression(expressionNode, 0);

	parent->AddChild(expressionNode);
}

/*
* subexpr -> (simpleexp | unop subexpr) { binop subexpr }
*/
void LuaParser::subexpression(std::shared_ptr<LuaAstNode> expressionNode, int limit)
{
	// ���趼�Ƕ�Ԫ���ʽ
	auto binaryExpression = createAstNode(LuaAstNodeType::BinaryExpression);

	UnOpr uop = getUnaryOperator(_tokenParser->Current().TokenType);
	if (uop != UnOpr::OPR_NOUNOPR) /* prefix (unary) operator? */
	{
		auto unaryExpression = createAstNode(LuaAstNodeType::UnaryExpression);

		auto unaryTokenNode = createAstNodeFromToken(LuaAstNodeType::UnaryOperator, _tokenParser->Current());
		unaryExpression->AddChild(unaryTokenNode);

		_tokenParser->Next();
		subexpression(unaryExpression, UNARY_PRIORITY);

		binaryExpression->AddChild(unaryExpression);
	}
	else
	{
		simpleExpression(binaryExpression);
	}


	bool isBinary = false;

	auto op = getBinaryOperator(_tokenParser->Current().TokenType);
	/* expand while operators have priorities higher than 'limit' */
	while (op != BinOpr::OPR_NOBINOPR && priority[static_cast<int>(op)].left > limit)
	{
		if (isBinary)
		{
			auto topBinaryExpression = createAstNode(LuaAstNodeType::BinaryExpression);
			topBinaryExpression->AddChild(binaryExpression);
			binaryExpression = topBinaryExpression;
		}
		else
		{
			isBinary = true;
		}

		auto binaryOperator = createAstNodeFromToken(LuaAstNodeType::BinaryOperator, _tokenParser->Current());
		binaryExpression->AddChild(binaryOperator);

		_tokenParser->Next();
		subexpression(binaryExpression, priority[static_cast<int>(op)].right);

		// next op
		op = getBinaryOperator(_tokenParser->Current().TokenType);
	}


	if (isBinary)
	{
		expressionNode->AddChild(binaryExpression);
	}
	else
	{
		auto& children = binaryExpression->GetChildren();
		for (auto child : children)
		{
			expressionNode->AddChild(child);
		}
	}
}

/* simpleexp -> FLT | INT | STRING | NIL | TRUE | FALSE | ... |
				constructor | FUNCTION body | suffixedexp */
void LuaParser::simpleExpression(std::shared_ptr<LuaAstNode> expressionNode)
{
	switch (_tokenParser->Current().TokenType)
	{
	case TK_FLT:
	case TK_INT:
	case TK_STRING:
	case TK_NIL:
	case TK_TRUE:
	case TK_FALSE:
	case TK_DOTS:
		{
			auto tokenNode = createAstNodeFromToken(LuaAstNodeType::LiteralExpression, _tokenParser->Current());
			expressionNode->AddChild(tokenNode);
			_tokenParser->Next();
			break;
		}
	case '{':
		{
			tableConstructor(expressionNode);
			break;
		}
	case TK_FUNCTION:
		{
			auto closureNode = createAstNode(LuaAstNodeType::ClosureExpression);
			auto functionKeyWordNode = createAstNodeFromToken(LuaAstNodeType::KeyWord, _tokenParser->Current());
			closureNode->AddChild(functionKeyWordNode);

			_tokenParser->Next();
			functionBody(closureNode);

			expressionNode->AddChild(closureNode);
			break;
		}
	default:
		{
			suffixedExpression(expressionNode);
			return;
		}
	}
}

/* constructor -> '{' [ field { sep field } [sep] ] '}'
   sep -> ',' | ';'
*/
void LuaParser::tableConstructor(std::shared_ptr<LuaAstNode> expressionNode)
{
	auto tableExpression = createAstNode(LuaAstNodeType::TableExpression);
	checkAndNext('{', tableExpression, LuaAstNodeType::GeneralOperator);

	do
	{
		if (_tokenParser->Current().TokenType == '}')
		{
			break;
		}
		field(tableExpression);
	}
	while (testNext(',', tableExpression, LuaAstNodeType::TableFieldSep) 
		|| testNext(';', tableExpression, LuaAstNodeType::TableFieldSep));

	checkMatch('}', '{', tableExpression);

	expressionNode->AddChild(tableExpression);
}

/* field -> listfield | recfield */
void LuaParser::field(std::shared_ptr<LuaAstNode> tableExpressionNode)
{
	auto tableFieldNode = createAstNode(LuaAstNodeType::TableField);
	switch (_tokenParser->Current().TokenType)
	{
	case TK_NAME:
		{
			if (_tokenParser->LookAhead().TokenType != '=')
			{
				listField(tableFieldNode);
			}
			else
			{
				rectField(tableFieldNode);
			}
			break;
		}
	case '[':
		{
			rectField(tableFieldNode);
			break;
		}
	default:
		{
			listField(tableFieldNode);
			break;
		}
	}

	tableExpressionNode->AddChild(tableFieldNode);
}

void LuaParser::listField(std::shared_ptr<LuaAstNode> tableFieldNode)
{
	expression(tableFieldNode);
}

/* recfield -> (NAME | '['exp']') = exp */
void LuaParser::rectField(std::shared_ptr<LuaAstNode> tableFieldNode)
{
	if (_tokenParser->Current().TokenType == TK_NAME)
	{
		codeName(tableFieldNode);
	}
	else
	{
		yIndex(tableFieldNode);
	}

	checkAndNext('=', tableFieldNode, LuaAstNodeType::GeneralOperator);

	expression(tableFieldNode);
}

/* body ->  '(' parlist ')' block END */
void LuaParser::functionBody(std::shared_ptr<LuaAstNode> closureExpression)
{
	auto functionBodyNode = createAstNode(LuaAstNodeType::FunctionBody);

	paramList(functionBodyNode);

	block(functionBodyNode);

	checkMatch(TK_END, TK_FUNCTION, functionBodyNode);

	closureExpression->AddChild(functionBodyNode);
}

void LuaParser::paramList(std::shared_ptr<LuaAstNode> functionBodyNode)
{
	auto paramList = createAstNode(LuaAstNodeType::ParamList);

	checkAndNext('(', paramList, LuaAstNodeType::GeneralOperator);

	bool isVararg = false;
	if (_tokenParser->Current().TokenType != ')')
	{
		do
		{
			switch (_tokenParser->Current().TokenType)
			{
			case TK_NAME:
				{
					auto paramNode = createAstNodeFromToken(LuaAstNodeType::Param, _tokenParser->Current());
					paramList->AddChild(paramNode);
					_tokenParser->Next();
					break;
				}
			case TK_DOTS:
				{
					isVararg = true;
					auto paramNode = createAstNodeFromToken(LuaAstNodeType::Param, _tokenParser->Current());
					paramList->AddChild(paramNode);
					_tokenParser->Next();
					break;
				}
			default:
				{
					luaError("<name> or '...' expected", paramList);
				}
			}
		}
		while (!isVararg && testNext(',', paramList, LuaAstNodeType::GeneralOperator));
	}

	checkAndNext(')', paramList, LuaAstNodeType::GeneralOperator);

	functionBodyNode->AddChild(paramList);
}

/* suffixedexp ->
	 primaryexp { '.' NAME | '[' exp ']' | ':' NAME funcargs | funcargs } */
void LuaParser::suffixedExpression(std::shared_ptr<LuaAstNode> expressionNode)
{
	auto subExpression = createAstNode(LuaAstNodeType::Expression);

	primaryExpression(subExpression);
	for (;;)
	{
		switch (_tokenParser->Current().TokenType)
		{
		case '.':
			{
				fieldSel(subExpression);
				break;
			}
		case '[':
			{
				yIndex(subExpression);
				break;
			}
		case ':':
			{
				fieldSel(subExpression);
				functionCallArgs(subExpression);
				break;
			}
		case '(':
		case TK_STRING:
		case '{':
			{
				functionCallArgs(subExpression);
				break;
			}
		default:
			expressionNode->AddChild(subExpression);
			return;
		}
	}
}

void LuaParser::functionCallArgs(std::shared_ptr<LuaAstNode> expressionNode)
{
	auto callArgsNode = createAstNode(LuaAstNodeType::CallArgList);
	switch (_tokenParser->Current().TokenType)
	{
	case '(':
		{
			auto tokenNode = createAstNodeFromToken(LuaAstNodeType::KeyWord, _tokenParser->Current());
			callArgsNode->AddChild(tokenNode);
			_tokenParser->Next();
			if (_tokenParser->Current().TokenType != ')')
			{
				expressionList(callArgsNode);
			}

			checkMatch(')', '(', callArgsNode);
			break;
		}
	case '{':
		{
			tableConstructor(callArgsNode);
			break;
		}
	case TK_STRING:
		{
			auto stringNode = createAstNodeFromToken(LuaAstNodeType::LiteralExpression, _tokenParser->Current());
			callArgsNode->AddChild(stringNode);
			_tokenParser->Next();
			break;
		}
	default:
		{
			luaError("function arguments expected", callArgsNode);
		}
	}

	expressionNode->AddChild(callArgsNode);
}

/* fieldsel -> ['.' | ':'] NAME */
void LuaParser::fieldSel(std::shared_ptr<LuaAstNode> expressionNode)
{
	auto tokenNode = createAstNodeFromToken(LuaAstNodeType::IndexOperator, _tokenParser->Current());
	expressionNode->AddChild(tokenNode);

	_tokenParser->Next();
	codeName(expressionNode);
}

/* index -> '[' expr ']' */
void LuaParser::yIndex(std::shared_ptr<LuaAstNode> expressionNode)
{
	checkAndNext('[', expressionNode, LuaAstNodeType::IndexOperator);

	expression(expressionNode);
	checkAndNext(']', expressionNode, LuaAstNodeType::IndexOperator);
}

void LuaParser::functionName(std::shared_ptr<LuaAstNode> functionNode)
{
	auto nameExpression = createAstNode(LuaAstNodeType::NameExpression);

	checkName(nameExpression);

	while (_tokenParser->Current().TokenType == '.')
	{
		fieldSel(nameExpression);
	}
	if (_tokenParser->Current().TokenType == ':')
	{
		fieldSel(nameExpression);
	}

	functionNode->AddChild(nameExpression);
}

std::string_view LuaParser::checkName(std::shared_ptr<LuaAstNode> parent)
{
	check(TK_NAME);

	auto identify = createAstNodeFromCurrentToken(LuaAstNodeType::Identify);

	parent->AddChild(identify);

	_tokenParser->Next();

	return identify->GetText();
}

/* ATTRIB -> ['<' Name '>'] */
LuaAttribute LuaParser::getLocalAttribute(std::shared_ptr<LuaAstNode> nameDefList)
{
	auto attribute = createAstNode(LuaAstNodeType::Attribute);

	if (testNext('<', attribute, LuaAstNodeType::GeneralOperator))
	{
		auto attributeName = checkName(attribute);
		checkAndNext('>', attribute, LuaAstNodeType::GeneralOperator);

		nameDefList->AddChild(attribute);

		if (attributeName == "const")
		{
			return LuaAttribute::Const;
		}
		else if (attributeName == "close")
		{
			return LuaAttribute::Close;
		}

		luaMatchError(format("unknown attribute {}", attributeName), attribute->GetTextRange());
	}

	return LuaAttribute::NoAttribute;
}

void LuaParser::check(LuaTokenType c)
{
	if (_tokenParser->Current().TokenType != c)
	{
		luaMatchError(format("{} expected", c), _tokenParser->Current().TextRange);
	}
}


/* primaryexp -> NAME | '(' expr ')' */
void LuaParser::primaryExpression(std::shared_ptr<LuaAstNode> expressionNode)
{
	switch (_tokenParser->Current().TokenType)
	{
	case '(':
		{
			auto leftBreaketToken = createAstNodeFromCurrentToken(LuaAstNodeType::KeyWord);
			expressionNode->AddChild(leftBreaketToken);
			_tokenParser->Next();

			expression(expressionNode);
			checkMatch(')', '(', expressionNode);
			return;
		}
	case TK_NAME:
		{
			codeName(expressionNode);
			return;
		}
	default:
		luaError("unexpected symbol", expressionNode);
	}
}

UnOpr LuaParser::getUnaryOperator(LuaTokenType op)
{
	switch (op)
	{
	case TK_NOT:
		{
			return UnOpr::OPR_NOT;
		}
	case '-':
		{
			return UnOpr::OPR_MINUS;
		}
	case '~':
		{
			return UnOpr::OPR_BNOT;
		}
	case '#':
		{
			return UnOpr::OPR_LEN;
		}
	default:
		{
			return UnOpr::OPR_NOUNOPR;
		}
	}
}

BinOpr LuaParser::getBinaryOperator(LuaTokenType op)
{
	switch (op)
	{
	case '+': return BinOpr::OPR_ADD;
	case '-': return BinOpr::OPR_SUB;
	case '*': return BinOpr::OPR_MUL;
	case '%': return BinOpr::OPR_MOD;
	case '^': return BinOpr::OPR_POW;
	case '/': return BinOpr::OPR_DIV;
	case TK_IDIV: return BinOpr::OPR_IDIV;
	case '&': return BinOpr::OPR_BAND;
	case '|': return BinOpr::OPR_BOR;
	case '~': return BinOpr::OPR_BXOR;
	case TK_SHL: return BinOpr::OPR_SHL;
	case TK_SHR: return BinOpr::OPR_SHR;
	case TK_CONCAT: return BinOpr::OPR_CONCAT;
	case TK_NE: return BinOpr::OPR_NE;
	case TK_EQ: return BinOpr::OPR_EQ;
	case '<': return BinOpr::OPR_LT;
	case TK_LE: return BinOpr::OPR_LE;
	case '>': return BinOpr::OPR_GT;
	case TK_GE: return BinOpr::OPR_GE;
	case TK_AND: return BinOpr::OPR_AND;
	case TK_OR: return BinOpr::OPR_OR;
	default: return BinOpr::OPR_NOBINOPR;
	}
}

void LuaParser::checkAndNext(LuaTokenType c, std::shared_ptr<LuaAstNode> parent, LuaAstNodeType addType)
{
	if (_tokenParser->Current().TokenType != c)
	{
		luaError(format("token type {} expected", c), parent);
		return;
	}

	parent->AddChild(createAstNodeFromCurrentToken(addType));

	_tokenParser->Next();
}

bool LuaParser::testNext(LuaTokenType c, std::shared_ptr<LuaAstNode> parent, LuaAstNodeType addType)
{
	if (_tokenParser->Current().TokenType == c)
	{
		parent->AddChild(createAstNodeFromToken(addType, _tokenParser->Current()));
		_tokenParser->Next();
		return true;
	}
	return false;
}

void LuaParser::codeName(std::shared_ptr<LuaAstNode> parent)
{
	if (_tokenParser->Current().TokenType == TK_NAME)
	{
		auto identify = createAstNodeFromToken(LuaAstNodeType::Identify, _tokenParser->Current());
		parent->AddChild(identify);
		_tokenParser->Next();
	}
}


std::shared_ptr<LuaAstNode> LuaParser::createAstNode(LuaAstNodeType type)
{
	return std::make_shared<LuaAstNode>(type, _tokenParser->GetSource().c_str());
}

std::shared_ptr<LuaAstNode> LuaParser::createAstNodeFromToken(LuaAstNodeType type, LuaToken& token)
{
	return std::make_shared<LuaAstNode>(type, token);
}

std::shared_ptr<LuaAstNode> LuaParser::createAstNodeFromCurrentToken(LuaAstNodeType type)
{
	return createAstNodeFromToken(type, _tokenParser->Current());
}

void LuaParser::luaError(std::string_view message, std::shared_ptr<LuaAstNode> parent)
{
	if (_tokenParser->Current().TokenType != TK_EOS)
	{
		auto tokenNode = createAstNodeFromCurrentToken(LuaAstNodeType::Error);
		parent->AddChild(tokenNode);
		_tokenParser->Next();
		_errors.emplace_back(message, tokenNode->GetTextRange());
	}
	else
	{
		auto offset = _tokenParser->LastValidOffset();
		if (offset != 0)
		{
			_errors.emplace_back(message, TextRange(offset, offset));
		}
	}
}

void LuaParser::luaMatchError(std::string message, TextRange range)
{
	_errors.emplace_back(message, range);
}

void LuaParser::forNumber(std::shared_ptr<LuaAstNode> forStatement)
{
	auto forNumberNode = createAstNode(LuaAstNodeType::ForNumber);

	checkName(forNumberNode);

	checkAndNext('=', forNumberNode, LuaAstNodeType::GeneralOperator);

	expression(forNumberNode);

	checkAndNext(',', forNumberNode, LuaAstNodeType::GeneralOperator);

	expression(forNumberNode);

	if (testNext(',', forNumberNode, LuaAstNodeType::GeneralOperator)) // optional step
	{
		expression(forNumberNode);
	}

	forBody(forNumberNode);

	forStatement->AddChild(forNumberNode);
}

/* forlist -> NAME {,NAME} IN explist forbody */
void LuaParser::forList(std::shared_ptr<LuaAstNode> forStatement)
{
	auto forListNode = createAstNode(LuaAstNodeType::ForList);

	auto nameDefList = createAstNode(LuaAstNodeType::NameDefList);

	checkName(nameDefList);

	while (testNext(',', nameDefList, LuaAstNodeType::GeneralOperator))
	{
		checkName(nameDefList);
	}

	forListNode->AddChild(nameDefList);

	checkAndNext(TK_IN, forListNode);

	expressionList(forListNode);

	forBody(forListNode);

	forStatement->AddChild(forListNode);
}

void LuaParser::forBody(std::shared_ptr<LuaAstNode> forNode)
{
	auto forBodyNode = createAstNode(LuaAstNodeType::ForBody);

	checkAndNext(TK_DO, forBodyNode);

	block(forBodyNode);

	checkMatch(TK_END, TK_FOR, forBodyNode);

	forNode->AddChild(forBodyNode);
}
