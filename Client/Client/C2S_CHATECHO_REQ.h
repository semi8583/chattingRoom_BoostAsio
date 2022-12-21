#pragma once
#define ECHO_REQ_BUF_SIZE 1024
#include <iostream>// memset, strlen 사용 하기 위해 필요
class C2S_CHATECHO_REQ
{
private:
	int size;
	int code = 0;
	int userIdx;
	int stringLength;
	char msg[ECHO_REQ_BUF_SIZE] = { 0, };
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
	void SetStringLegnth(int _stringLength);
	char* GetMsg();
	void SetMsg(char * msg);
};