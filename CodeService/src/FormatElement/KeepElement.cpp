#include "CodeService/FormatElement/KeepElement.h"
#include "Util/format.h"

KeepElement::KeepElement(int keepBlank, bool hasLinebreak)
	: _keepBlank(keepBlank),
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

void KeepElement::Diagnosis(DiagnosisContext& ctx, int position, FormatElement& parent)
{
	if(position == 0)
	{
		return;
	}

	int lastOffset = getLastValidOffset(position, parent);
	int nextOffset = getNextValidOffset(position, parent);

	if (nextOffset == -1)
	{
		return;
	}

	int lastElementLine = ctx.GetLine(lastOffset);
	int nextElementLine = ctx.GetLine(nextOffset);

	if (nextElementLine == lastElementLine)
	{
		if (nextOffset - lastOffset - 1 != _keepBlank)
		{
			ctx.PushDiagnosis(format(LText("here need keep {} space"), _keepBlank), TextRange(lastOffset, nextOffset));
		}
	}
	else
	{
		ctx.SetCharacterCount(0);
	}
}
