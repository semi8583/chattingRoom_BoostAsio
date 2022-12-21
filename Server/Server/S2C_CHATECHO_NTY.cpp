#include "S2C_CHATECHO_NTY.h"
char* S2C_CHATECHO_NTY::Serialize(int _size, int _code, int _userIdx, int _stringLength, char* _msg)
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

void S2C_CHATECHO_NTY::Deserialize(char* _buffer)
{
    memset(this->msg, 0, ECHO_NTY_BUF_SIZE); // 기존 멤버 변수 초기화
    memcpy(&this->size, &(_buffer[0]), sizeof(int));
    memcpy(&this->code, &(_buffer[4]), sizeof(int));
    memcpy(&this->userIdx, &(_buffer[8]), sizeof(int));
    memcpy(&this->stringLength, &(_buffer[12]), sizeof(int));

    for (int j = 0; j < this->stringLength; j++)
        this->msg[j] = _buffer[j + 16];
}

int S2C_CHATECHO_NTY::GetSize()
{
    return this->size;
}
void S2C_CHATECHO_NTY::SetSize(int _size)
{
    this->size = _size;
}
int S2C_CHATECHO_NTY::GetCode()
{
    return this->code;
}
void S2C_CHATECHO_NTY::SetCode(int _code)
{
    this->code = _code;
}
int S2C_CHATECHO_NTY::GetUserIdx()
{
    return this->userIdx;
}
void S2C_CHATECHO_NTY::SetUserIdx(int _userIdx)
{
    this->userIdx = _userIdx;
}
int S2C_CHATECHO_NTY::GetStringLength()
{
    return this->stringLength;
}
void S2C_CHATECHO_NTY::SetStringLegnth(int _stringLegnth)
{
    this->stringLength = _stringLegnth;
}
char* S2C_CHATECHO_NTY::GetMsg()
{
    return this->msg;
}
void S2C_CHATECHO_NTY::SetMsg(char* _msg)
{
    memset(this->msg, 0, ECHO_NTY_BUF_SIZE);
    for (int j = 0; j < strlen(_msg); j++)
        this->msg[j] = _msg[j];
}