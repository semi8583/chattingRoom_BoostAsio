#include "S2C_PID_ACK.h"
char* S2C_PID_ACK::Serialize(int _size, int _code, int _pid)
{
	char* newBuffer = new char[_size];

	memset(newBuffer, 0, _size);

	memcpy(&(newBuffer[0]), &_size, sizeof(_size));
	memcpy(&(newBuffer[4]), &_code, sizeof(_code));
	memcpy(&(newBuffer[8]), &_pid, sizeof(_pid));
	return newBuffer;
}

void S2C_PID_ACK::Deserialize(char* _buffer)
{
	memcpy(&this->size, &(_buffer[0]), sizeof(int));
	memcpy(&this->code, &(_buffer[4]), sizeof(int));
	memcpy(&this->pid, &(_buffer[8]), sizeof(int));
}
int S2C_PID_ACK::GetSize()
{
	return this->size;
}
void S2C_PID_ACK::SetSize(int _size)
{
	this->size = _size;
}
int S2C_PID_ACK::GetCode()
{
	return this->code;
}
void S2C_PID_ACK::SetCode(int _code)
{
	this->code = _code;
}
int S2C_PID_ACK::GetPid()
{
	return this->pid;
}
void S2C_PID_ACK::SetPid(int _pid)
{
	this->pid = _pid;
}