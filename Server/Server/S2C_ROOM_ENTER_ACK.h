#pragma once
#define ROOM_ACK_BUF_SIZE 1024
#include <iostream>// memset, strlen 사용 하기 위해 필요
class S2C_ROOM_ENTER_ACK
{
private:
	int size;
	int code = 0;
	int roomNo = 0;
	int result;
public:
	char* Serialize(int _size, int _code, int _roomNo, int _result);
	void Deserialize(char* _buffer);

	int GetSize();
	void SetSize(int _size);
	int GetCode();
	void SetCode(int _code);
	int GetRoomNo();
	void SetRoomNo(int _roomNo);
	int GetResult();
	void SetResult(int _result);
};