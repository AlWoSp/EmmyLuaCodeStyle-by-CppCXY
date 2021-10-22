#pragma once

#include "KeepLineElement.h"

class KeepElement : public KeepLineElement
{
public:
	/*
	 * @param keepBlank ��ʾ�����һ��Ԫ�غ͵�ǰԪ����ͬһ���򱣳ּ����ո�
	 * @remark �����һ��Ԫ�غ͵�ǰԪ���ڲ�ͬ������
	 */
	KeepElement(int keepBlank);

	FormatElementType GetType() override;

	void Serialize(FormatContext& ctx, int position, FormatElement* parent) override;
private:
	int _keepBlank;
};
