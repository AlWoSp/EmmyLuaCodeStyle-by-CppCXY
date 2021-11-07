#pragma once

#include <string>

class ProtocolBuffer
{
public:
	ProtocolBuffer(std::size_t capacity);

	char* GetWritableCursor();

	std::size_t GetRestCapacity();

	void SetReadableSize(std::size_t readableSize);

	bool CanReadOneProtocol();

	std::string_view ReadOneProtocol();

	void Reset();
private:
	bool TryParseHead();
	// ��������λ�ÿ�ʼ��д
	std::size_t _writeIndex;
	// ��������λ�ÿ�ʼ�ɶ�
	// ͬʱ���writeIndex �� startIndex ��ͬ����Ϊ�ɶ�����Ϊ0
	std::size_t _startIndex;
	std::string _buffer;

	std::size_t _contentLength;
	std::size_t _bodyStartIndex;
};

