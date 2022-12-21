#include "S2C_ROOM_ENTER_ACK.h"
char* S2C_ROOM_ENTER_ACK::Serialize(int _size, int _code, int _roomNo, int _result)
{
	char* newBuffer = new char[_size];

	memset(newBuffer, 0, _size);

	memcpy(&(newBuffer[0]), &_size, sizeof(_size));
	memcpy(&(newBuffer[4]), &_code, sizeof(_code));
	memcpy(&(newBuffer[8]), &_roomNo, sizeof(_roomNo));
	memcpy(&(newBuffer[12]), &_result, sizeof(_result));

	return newBuffer;
}
void S2C_ROOM_ENTER_ACK::Deserialize(char* _buffer)
{
	memcpy(&this->size, &(_buffer[0]), sizeof(int));
	memcpy(&this->code, &(_buffer[4]), sizeof(int));
	memcpy(&this->roomNo, &(_buffer[8]), sizeof(int));
	memcpy(&this->result, &(_buffer[12]), sizeof(int));
}

int S2C_ROOM_ENTER_ACK::GetSize()
{
	return this->size;
}
void S2C_ROOM_ENTER_ACK::SetSize(int _size)
{
	this->size = _size;
}
int S2C_ROOM_ENTER_ACK::GetCode()
{
	return this->code;
}
void S2C_ROOM_ENTER_ACK::SetCode(int _code)
{
	this->code = _code;
}
int S2C_ROOM_ENTER_ACK::GetRoomNo()
{
	return this->roomNo;
}
void S2C_ROOM_ENTER_ACK::SetRoomNo(int _roomNo)
{
	this->roomNo = _roomNo;
}
int S2C_ROOM_ENTER_ACK::GetResult()
{
	return this->result;
}
void S2C_ROOM_ENTER_ACK::SetResult(int _result)
{
	this->result = _result;
}