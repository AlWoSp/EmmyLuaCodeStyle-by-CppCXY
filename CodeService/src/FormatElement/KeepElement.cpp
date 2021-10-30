#include "CodeService/FormatElement/KeepElement.h"

KeepElement::KeepElement(int keepBlank, bool hasLinebreak)
	: KeepLineElement(0),
	  _keepBlank(keepBlank),
	  _hasLinebreak(hasLinebreak)
{
}

FormatElementType KeepElement::GetType()
{
	return FormatElementType::KeepElement;
}

void KeepElement::Serialize(FormatContext& ctx, int position, FormatElement& parent)
{
	int lastElementLine = getLastValidLine(ctx, position, parent);
	int nextElementLine = getNextValidLine(ctx, position, parent);

	if (nextElementLine == -1)
	{
		return;
	}
	// �����������˼�������һ��Ԫ�غ���һ��Ԫ��û��ʵ�ʵĻ����򱣳�һ���Ŀո�
	if (nextElementLine == lastElementLine && ctx.GetCharacterCount() != 0)
	{
		ctx.PrintBlank(_keepBlank);
	}
	else
	{
		int line = nextElementLine - lastElementLine;
		if (_hasLinebreak)
		{
			line--;
		}
		ctx.PrintLine(line);
	}
}
