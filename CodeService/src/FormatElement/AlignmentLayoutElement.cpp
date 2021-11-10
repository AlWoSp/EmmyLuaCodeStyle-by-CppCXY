#include "CodeService/FormatElement/AlignmentLayoutElement.h"
#include "CodeService/FormatElement/StatementElement.h"
#include "CodeService/FormatElement/TextElement.h"
#include "CodeService/FormatElement/AlignmentElement.h"

int FindLastValidChildIndex(int position, std::vector<std::shared_ptr<FormatElement>>& vec)
{
	for (int index = position - 1; position >= 0; index--)
	{
		if (vec[index]->HasValidTextRange())
		{
			return index;
		}
	}
	return -1;
}


FormatElementType AlignmentLayoutElement::GetType()
{
	return FormatElementType::AlignmentLayoutElement;
}

void AlignmentLayoutElement::Serialize(FormatContext& ctx, int position, FormatElement& parent)
{
	auto eqPosition = getAlignPosition(ctx, position, parent);

	if (eqPosition != -1)
	{
		return alignmentSerialize(ctx, position, parent, eqPosition);
	}
	else
	{
		FormatElement::Serialize(ctx, position, parent);
	}
}

void AlignmentLayoutElement::Diagnosis(FormatContext& ctx, int position, FormatElement& parent)
{
	auto eqPosition = getAlignPosition(ctx, position, parent);

	if (eqPosition != -1)
	{
		return alignmentDiagnosis(ctx, position, parent, eqPosition);
	}
	else
	{
		FormatElement::Diagnosis(ctx, position, parent);
	}
}

int AlignmentLayoutElement::getAlignPosition(FormatContext& ctx, int position, FormatElement& parent)
{
	int eqAlignedPosition = 0;
	bool firstContainEq = true;
	// ��Ѱ�ҵȺŶ����λ�ã������ж������Ĵ��Ⱥŵ�����Ƿ�Ӧ�ö��뵽�Ⱥ�
	// �����Ĵ��Ⱥŵ�����Ƿ�Ӧ�ö��뵽�Ⱥţ������ΪӦ���������������о���
	// ������ӽڵ��ڵ�������乲ͬ��������ѽ��������뻹ԭΪ��ͨ�Ű�
	for (int statIndex = 0; statIndex != static_cast<int>(_children.size()); statIndex++)
	{
		auto statChild = _children[statIndex];

		auto& statChildren = statChild->GetChildren();

		for (int index = 0; index != static_cast<int>(statChildren.size()); index++)
		{
			auto textChild = statChildren[index];
			if (textChild->GetType() == FormatElementType::TextElement)
			{
				auto textElement = std::dynamic_pointer_cast<TextElement>(textChild);
				if (textElement->GetText() == "=")
				{
					int eqPosition = ctx.GetColumn(textElement->GetTextRange().StartOffset);
					if (firstContainEq && index > 0)
					{
						firstContainEq = false;
						auto lastValidIndex = FindLastValidChildIndex(index, statChildren);
						if (lastValidIndex == -1)
						{
							return -1;
						}
						auto lastStatChild = statChildren[lastValidIndex];
						auto lastPosition = ctx.GetColumn(lastStatChild->GetTextRange().EndOffset);


						if (eqPosition - lastPosition <= 2)
						{
							return -1;
						}
					}

					if (eqAlignedPosition < eqPosition)
					{
						eqAlignedPosition = eqPosition;
					}
				}
			}
		}
	}

	return eqAlignedPosition;
}

void AlignmentLayoutElement::alignmentSerialize(FormatContext& ctx, int position, FormatElement& parent, int eqPosition)
{
	for (auto statChild : _children)
	{
		auto& statChildren = statChild->GetChildren();

		for (int i = 0; i < static_cast<int>(statChildren.size()); i++)
		{
			auto textChild = statChildren[i];
			if (textChild->GetType() == FormatElementType::TextElement)
			{
				auto textElement = std::dynamic_pointer_cast<TextElement>(textChild);
				if (textElement->GetText() == "=" && i > 0)
				{
					// ������Ԫ�ر��Ϊ����Ԫ��
					statChildren[i - 1] = std::make_shared<AlignmentElement>(eqPosition);
					break;
				}
			}
		}
	}

	return FormatElement::Serialize(ctx, position, parent);
}

void AlignmentLayoutElement::alignmentDiagnosis(FormatContext& ctx, int position, FormatElement& parent, int eqPosition)
{
	for (auto statChild : _children)
	{
		auto& statChildren = statChild->GetChildren();

		for (int i = 0; i < static_cast<int>(statChildren.size()); i++)
		{
			auto textChild = statChildren[i];
			if (textChild->GetType() == FormatElementType::TextElement)
			{
				auto textElement = std::dynamic_pointer_cast<TextElement>(textChild);
				if (textElement->GetText() == "=" && i > 0)
				{
					// ������Ԫ�ر��Ϊ����Ԫ��
					statChildren[i - 1] = std::make_shared<AlignmentElement>(eqPosition);
					break;
				}
			}
		}
	}

	return FormatElement::Diagnosis(ctx, position, parent);
}
