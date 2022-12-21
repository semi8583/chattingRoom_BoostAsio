#include "C2S_ROOM_ENTER_REQ.h"
char* C2S_ROOM_ENTER_REQ::Serialize(int _size, int _code, int _roomNo, int _userIdx)
{
	char* newBuffer = new char[_size];

	memset(newBuffer, 0, _size);

	memcpy(&(newBuffer[0]), &_size, sizeof(_size));
	memcpy(&(newBuffer[4]), &_code, sizeof(_code));
	memcpy(&(newBuffer[8]), &_roomNo, sizeof(_roomNo));
	memcpy(&(newBuffer[12]), &_userIdx, sizeof(_userIdx));

	return newBuffer;
}

void C2S_ROOM_ENTER_REQ::Deserialize(char* _buffer)
{
	memcpy(&this->size, &(_buffer[0]), sizeof(int));
	memcpy(&this->code, &(_buffer[4]), sizeof(int));
	memcpy(&this->roomNo, &(_buffer[8]), sizeof(int));
	memcpy(&this->userIdx, &(_buffer[12]), sizeof(int));
}

int C2S_ROOM_ENTER_REQ::GetSize()
{
	return this->size;
}
void C2S_ROOM_ENTER_REQ::SetSize(int _size)
{
	this->size = _size;
}
int C2S_ROOM_ENTER_REQ::GetCode()
{
	return this->code;
}
void C2S_ROOM_ENTER_REQ::SetCode(int _code)
{
	this->code = _code;
}
int C2S_ROOM_ENTER_REQ::GetRoomNo()
{
	return this->roomNo;
}

void C2S_ROOM_ENTER_REQ::SetRoomNo(int _roomNo)
{
	this->roomNo = _roomNo;
}

int C2S_ROOM_ENTER_REQ::GetUserIdx()
{
	return this->userIdx;
}
void C2S_ROOM_ENTER_REQ::SetUserIdx(int _userIdx)
{
	this->userIdx = _userIdx;
}