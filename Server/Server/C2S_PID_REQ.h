#pragma once
#include <iostream>// memset, strlen 사용 하기 위해 필요
class C2S_PID_REQ
{
private:
	int size;
	int code = 0;
	int pid;
public:
	char* Serialize(int _size, int _code, int _pid);
	void Deserialize(char* _buffer);

	int GetSize();
	void SetSize(int _size);
	int GetCode();
	void SetCode(int _code);
	int GetPid();
	void SetPid(int _pid);
};

