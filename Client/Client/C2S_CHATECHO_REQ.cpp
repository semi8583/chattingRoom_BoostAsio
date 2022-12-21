#include "C2S_CHATECHO_REQ.h"
#include <iostream>
using namespace std;
char* C2S_CHATECHO_REQ::Serialize(int _size, int _code, int _userIdx, int _stringLength, char* _msg)
{
	char* newBuffer = new char[_size];
	
	memset(newBuffer, 0, _size);

	memcpy(&(newBuffer[0]), &_size, sizeof(_size));
	memcpy(&(newBuffer[4]), &_code, sizeof(_code));
	memcpy(&(newBuffer[8]), &_userIdx, sizeof(_userIdx));
	memcpy(&(newBuffer[12]), &_stringLength, sizeof(_stringLength));

	for (int i = 0; i < _stringLength; i++)
		newBuffer[16 + i] = _msg[i];

	return newBuffer;
}

void C2S_CHATECHO_REQ::Deserialize(char* _buffer)
{
	memset(this->msg, 0, ECHO_REQ_BUF_SIZE); // 기존 멤버 변수 초기화
	memcpy(&this->size, &(_buffer[0]), sizeof(int));
	memcpy(&this->code, &(_buffer[4]), sizeof(int));
	memcpy(&(this->userIdx), &(_buffer[8]), sizeof(int));
	memcpy(&this->stringLength, &(_buffer[12]), sizeof(int));

	for (int j = 0; j < this->stringLength; j++)
		this->msg[j] = _buffer[j + 16];
}

int C2S_CHATECHO_REQ::GetSize()
{
	return this->size;
}
void C2S_CHATECHO_REQ::SetSize(int _size)
{
	this->size = _size;
}
int C2S_CHATECHO_REQ::GetCode()
{
	return this->code;
}
void C2S_CHATECHO_REQ::SetCode(int _code)
{
	this->code = _code;
}
int C2S_CHATECHO_REQ::GetUserIdx()
{
	return this->userIdx;
}
void C2S_CHATECHO_REQ::SetUserIdx(int _userIdx)
{
	this->userIdx = _userIdx;
}
int C2S_CHATECHO_REQ::GetStringLength()
{
	return this->stringLength;
}
void C2S_CHATECHO_REQ::SetStringLegnth(int _stringLength)
{
	this->stringLength = _stringLength;
}
char* C2S_CHATECHO_REQ::GetMsg()
{
	return this->msg;
}
void C2S_CHATECHO_REQ::SetMsg(char* _msg)
{
	memset(this->msg, 0, ECHO_REQ_BUF_SIZE);
	for (int j = 0; j < strlen(_msg); j++)
		this->msg[j] = _msg[j];
}