#pragma once

#include <string>

class ProtocolBuffer
{
public:
	ProtocolBuffer(std::size_t capacity);

	char* GetWritableCursor();

	std::size_t GetRestCapacity();

	void WriteBuff(std::size_t size);

	bool CanReadOneProtocol();

	std::string_view ReadOneProtocol();

	void Reset();
private:
	bool TryParseHead();
	// ��������λ�ÿ�ʼ��д
	std::size_t _writeIndex;
	std::string _textProtocol;

	std::string _readBuffer;

	std::size_t _contentLength;
	std::size_t _bodyStartIndex;


};

