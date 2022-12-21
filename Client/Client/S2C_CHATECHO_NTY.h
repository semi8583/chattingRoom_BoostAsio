#pragma once
#define ECHO_NTY_BUF_SIZE 1024
#include <iostream>// memset, strlen 사용 하기 위해 필요
class S2C_CHATECHO_NTY
{
private:
	int size;
	int code = 0;
	int userIdx;
	int stringLength;
	char msg[ECHO_NTY_BUF_SIZE] = { 0, };
public:
	char* Serialize(int _size, int _code, int _userIdx, int _stringLength, char* _msg);
	void Deserialize(char* _buffer);

	int GetSize();
	void SetSize(int _size);
	int GetCode();
	void SetCode(int _code);
	int GetUserIdx();
	void SetUserIdx(int _userIdx);
	int GetStringLength();
	void SetStringLegnth(int _stringLegnth);
	char* GetMsg();
	void SetMsg(char* _msg);
};