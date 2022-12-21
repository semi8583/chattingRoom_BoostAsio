#include "S2C_CHATECHO_ACK.h"

char* S2C_CHATECHO_ACK::Serialize(int _size, int _code, int _result, int _stringLength, char* _msg)
{
	char* newBuffer = new char[_size];

	memset(newBuffer, 0, _size);

	memcpy(&(newBuffer[0]), &_size, sizeof(_size));
	memcpy(&(newBuffer[4]), &_code, sizeof(_code));
	memcpy(&(newBuffer[8]), &_result, sizeof(_result));
	memcpy(&(newBuffer[12]), &_stringLength, sizeof(_stringLength));

	for (int i = 0; i < _stringLength; i++)
		newBuffer[16 + i] = _msg[i];

	return newBuffer;
}
void S2C_CHATECHO_ACK::Deserialize(char* _buffer)
{
	memset(this->msg, 0, ECHO_ACK_BUF_SIZE); // 기존 멤버 변수 초기화
	memcpy(&this->size, &(_buffer[0]), sizeof(int));
	memcpy(&this->code, &(_buffer[4]), sizeof(int));
	memcpy(&this->result, &(_buffer[8]), sizeof(int));
	memcpy(&this->stringLength, &(_buffer[12]), sizeof(int));

	for (int j = 0; j < this->stringLength; j++)
		this->msg[j] = _buffer[j + 16];
}
int S2C_CHATECHO_ACK::GetSize()
{
	return this->size;
}
void S2C_CHATECHO_ACK::SetSize(int _size)
{
	this->size = _size;
}
int S2C_CHATECHO_ACK::GetCode()
{
	return this->code;
}
void S2C_CHATECHO_ACK::SetCode(int _code)
{
	this->code = _code;
}
int S2C_CHATECHO_ACK::GetResult()
{
	return this->result;
}
void S2C_CHATECHO_ACK::SetResult(int _result)
{
	this->result = _result;
}
int S2C_CHATECHO_ACK::GetStringLength()
{
	return this->stringLength;
}
void S2C_CHATECHO_ACK::SetStringLength(int _stringLength)
{
	this->stringLength = _stringLength;
}
char* S2C_CHATECHO_ACK::GetMsg()
{
	return this->msg;
}
void S2C_CHATECHO_ACK::SetMsg(char* _msg)
{
	memset(this->msg, 0, ECHO_ACK_BUF_SIZE);
	for (int j = 0; j < strlen(_msg); j++)
		this->msg[j] = _msg[j];
}