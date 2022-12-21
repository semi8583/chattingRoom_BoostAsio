#include "S2C_ROOM_ENTER_NTY.h"
char* S2C_ROOM_ENTER_NTY::Serialize(int _size, int _code, int _roomNo, int _userIdx)
{
	char* newBuffer = new char[_size];

	memset(newBuffer, 0, _size);

	memcpy(&(newBuffer[0]), &_size, sizeof(_size));
	memcpy(&(newBuffer[4]), &_code, sizeof(_code));
	memcpy(&(newBuffer[8]), &_roomNo, sizeof(_roomNo));
	memcpy(&(newBuffer[12]), &_userIdx, sizeof(_userIdx));

	return newBuffer;
}
void S2C_ROOM_ENTER_NTY::Deserialize(char* _buffer)
{
	memcpy(&this->size, &(_buffer[0]), sizeof(int));
	memcpy(&this->code, &(_buffer[4]), sizeof(int));
	memcpy(&this->roomNo, &(_buffer[8]), sizeof(int));
	memcpy(&this->userIdx, &(_buffer[12]), sizeof(int));
}
int S2C_ROOM_ENTER_NTY::GetSize()
{
	return this->size;
}
void S2C_ROOM_ENTER_NTY::SetSize(int _size)
{
	this->size = _size;
}
int S2C_ROOM_ENTER_NTY::GetCode()
{
	return this->code;
}
void S2C_ROOM_ENTER_NTY::SetCode(int _code)
{
	this->code = _code;
}
int S2C_ROOM_ENTER_NTY::GetRoomNo()
{
	return this->roomNo;
}
void S2C_ROOM_ENTER_NTY::SetRoomNo(int _roomNo)
{
	this->roomNo = _roomNo;
}
int S2C_ROOM_ENTER_NTY::GetUserIdx()
{
	return this->userIdx;
}
void S2C_ROOM_ENTER_NTY::SetUserIdx(int _userIdx)
{
	this->userIdx = _userIdx;
}
