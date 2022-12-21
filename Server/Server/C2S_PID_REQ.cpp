#include "C2S_PID_REQ.h"
char *C2S_PID_REQ::Serialize(int _size, int _code, int _pid)
{
	char* newBuffer = new char[_size];

	memset(newBuffer, 0, _size);

	memcpy(&(newBuffer[0]), &_size, sizeof(_size));
	memcpy(&(newBuffer[4]), &_code, sizeof(_code));
	memcpy(&(newBuffer[8]), &_pid, sizeof(_pid));
	return newBuffer;
}

void C2S_PID_REQ::Deserialize(char* _buffer)
{
	memcpy(&this->size, &(_buffer[0]), sizeof(int));
	memcpy(&this->code, &(_buffer[4]), sizeof(int));
	memcpy(&this->pid, &(_buffer[8]), sizeof(int));
}
int C2S_PID_REQ::GetSize()
{
	return this->size;
}
void C2S_PID_REQ::SetSize(int _size)
{
	this->size = _size;
}
int C2S_PID_REQ::GetCode()
{
	return this->code;
}
void C2S_PID_REQ::SetCode(int _code)
{
	this->code = _code;
}
int C2S_PID_REQ::GetPid()
{
	return this->pid;
}
void C2S_PID_REQ::SetPid(int _pid)
{
	this->pid = _pid;
}