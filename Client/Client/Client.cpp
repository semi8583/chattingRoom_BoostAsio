#include <iostream>
#include <map>
#include <thread>
#include <charconv>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <boost/thread/mutex.hpp>
#include <SDKDDKVER.h> //이게 없으면 컴파일할 때 warning을 낸다.
//boost asio를 컴파일하는데, 컴파일하는 윈도우 라이브러리의 버전을 알 수 없다는 에러.
//이 헤더를 넣으면 경고가 사라지며 깔끔해진다.
#include "flatbuffers/flatbuffers.h"
#include "C2S_CHATECHO_REQ_generated.h"
#include "C2S_PID_REQ_generated.h"
#include "C2S_ROOM_ENTER_REQ_generated.h"
#include "S2C_CHATECHO_ACK_generated.h"
#include "S2C_CHATECHO_NTY_generated.h"
#include "S2C_PID_ACK_generated.h"
#include "S2C_ROOM_ENTER_ACK_generated.h"
#include "S2C_ROOM_ENTER_NTY_generated.h"

using namespace std;
using boost::asio::ip::tcp;

void MenuSelection();
void ChattingEcho();
void ChattingRoom();
void RecvCharEcho(char* buffer);
void RecvCharRoom(char* buffer);
void RecvCharPid(char* buffer);
void RecvCharValidRoomNo(char* buffer);
void Char_Recv(const boost::system::error_code& ec);

static CHAR IP[] = "127.0.0.1";
static CHAR Port[] = "3587";
#define BUF_SIZE 1024

//SOCKET hSocket; // 소켓 생성하는 함수
boost::asio::io_context io_context;
boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address::from_string(IP), atoi(Port));
boost::asio::ip::tcp::socket hSocket(io_context, ep.protocol()); // 소켓과 io_context가 등록
boost::thread_group threadGroup;
boost::system::error_code error;

bool threadStop = true;
bool mainLoop = false;
bool mainFinish = true;

int pid;

int MenuNum = 0;

enum Code
{
	NO_CHOICE,
	CHAT_ECHO,
	CHAT_ROOM,
	PID,
	VALID_ROOM_NO
};

map<int, void(*)(char*)> callbackMap =
{
	{1, RecvCharEcho},
	{2, RecvCharRoom},
	{3, RecvCharPid},
	{4, RecvCharValidRoomNo}
};

void WorkerThread()
{
	io_context.run();
}

INT main(int argc, char* argv[])
{
	threadStop = true;
	hSocket.async_connect(ep, Char_Recv);

	//for (int i = 0; i < 2; ++i) {
	//	thread{ [&]() {
	//		io_context.run();
	//	} }.detach(); // detach 스레드가 언제 종료될지 모른다. 
	//} // join -> 스레드가 종료되는 시점에 자원을 반환받는 것이 보장 

	for (int i = 0; i < 2; ++i) {
		threadGroup.create_thread(WorkerThread);
	} 

	while (mainFinish)
	{
		while (mainLoop)
		{
			MenuSelection();
			cin >> MenuNum;
			cin.ignore();// 입력 버퍼 초기화

			if (MenuNum == 1) // 채팅 에코
			{
				ChattingEcho();
			}
			else if (MenuNum == 2) // 채팅룸 입장
			{
				mainLoop = false;
				ChattingRoom();
			}
			else if (MenuNum == 0) // 프로그램종료f
			{
				mainLoop = false;
				mainFinish = false;
			}
			else
			{
				cout << "다시 입력해 주세요!" << endl;
			}
		}
	}
	threadStop = false;
	cout << "[ACK] 클라이언트 종료" << endl;

	return 0;
}

void MenuSelection()
{
	cout << "메뉴를 선택해 주세요!" << endl;
	cout << "1. 채팅 에코 메시지 전송" << endl;
	cout << "2. 채팅 룸 입장" << endl;
	cout << "0. 프로그램 종료" << endl;
}

void ChattingEcho()
{
	flatbuffers::FlatBufferBuilder builder;
	threadStop = true;
	cout << "\n채팅 에코 프로그램  " << endl;
	cout << "\n문자를 입력해주세요: (-1 입력 시 종료) ";
	while (1)
	{
		char input[BUF_SIZE] = { 0, };
		memset(input, 0, BUF_SIZE);
		cin.getline(input, BUF_SIZE, '\n');
		cin.clear();// 입력 버퍼 초기화

		if (input[0] == '-' && input[1] == '1')    //종료문자 처리
		{
			cout << "클라이언트 종료!" << endl;
			threadStop = false;
			break;
		}
		if (input != "")
			cout << "[ACK] 문자열 입력 성공!" << endl;
		int sendReturn;

		builder.Finish(CreateC2S_CHATECHO_REQ(builder, strlen(input) + 16, MenuNum, pid, strlen(input), builder.CreateString(input)));
		char tmpBuffer[3000] = { 0, };
		memcpy(&tmpBuffer, builder.GetBufferPointer(), builder.GetSize());

		hSocket.write_some(boost::asio::buffer(tmpBuffer), error);
		if (error)
		{
			sendReturn = 0;
			cout << "문자열 전송 실패" << endl;
			
		}
		else
		{
			sendReturn = 1;
			cout << "문자열 전송 성공" << endl;
		}
		builder.Clear();
		builder.Finish(CreateS2C_CHATECHO_ACK(builder, strlen(input) + 16, MenuNum, sendReturn, strlen(input), builder.CreateString(input)));
		auto s2cEchoAck = GetS2C_CHATECHO_ACK(builder.GetBufferPointer());
		cout << "[ACK] [send] " << "총 버퍼 사이즈: \"" << s2cEchoAck->size() << "\" Code(채팅 에코:1, 채팅 룸:2, 방 번호 유무:4): \"" << s2cEchoAck->code() << "\" Result(문자열 전송 성공: 1, 문자열 전송 실패: 0): \"" << s2cEchoAck->result() << "\" , 문자열 길이: \"" << s2cEchoAck->stringLength() << "\" , 문자열: \"" << s2cEchoAck->msg()->c_str() << "\" from " << pid << " 번째 client " << endl;
	}
	cout << "\n채팅 에코 프로그램  종료" << endl;
	builder.Clear();
}

void ChattingRoom()
{
	flatbuffers::FlatBufferBuilder builder;
	threadStop = true;
	cout << "\n채팅 룸 프로그램  " << endl;
	cout << "\n입장할 채팅 방 번호를 입력 하세요!: (입력 안할시 default: -1, -2 입력시 종료) ";

	char input[BUF_SIZE] = { 0, };
	memset(input, 0, BUF_SIZE);
	cin.getline(input, BUF_SIZE, '\n');
	cin.clear();// 입력 버퍼 초기화

	int roomNo;
	if (strlen(input) == 0 || atoi(input) == -1)
		roomNo = 0; // 방 번호 입력 안할시 -1번 방 선택
	else
		roomNo = atoi(input); // 1번방 또는 2번 방

	if (roomNo == -2)    //종료문자 처리
	{
		threadStop = false;
		cout << "\n채팅 룸 프로그램  종료" << endl;
		mainLoop = true;
	}
	else // 방 번호 서버에서 확인
	{
		builder.Finish(CreateC2S_ROOM_ENTER_REQ(builder, 16, 4, roomNo, pid));
		char tmpBuffer[BUF_SIZE] = { 0, };
		memcpy(&tmpBuffer, builder.GetBufferPointer(), builder.GetSize());
		hSocket.write_some(boost::asio::buffer(tmpBuffer), error);
	}
	builder.Clear();
}

void RecvCharEcho(char* buffer)
{
	flatbuffers::FlatBufferBuilder builder;
	auto s2cEchoNty = GetS2C_CHATECHO_NTY(buffer);
	builder.Finish(CreateS2C_CHATECHO_NTY(builder, s2cEchoNty->size() , s2cEchoNty->code(), s2cEchoNty->userIdx(), s2cEchoNty->stringLength(), builder.CreateString(s2cEchoNty->msg()->c_str())));

	cout << "\n[recv] msg received. 총 버퍼 사이즈 : \"" << s2cEchoNty->size() << "\", Code(채팅 에코:1, 채팅 룸:2, 방 번호 유무:4): \"" << s2cEchoNty->code() << "\", 문자열 길이: \"" << s2cEchoNty->stringLength() << "\" , 문자열: \"" << s2cEchoNty->msg()->c_str() << "\" from server \"" << s2cEchoNty->userIdx() << "\" 번째 Client" << endl;// 받은 숫자를 콘솔 창에 출력
	if (MenuNum == 1)
		cout << "\n문자를 입력해주세요: (-1 입력 시 종료) ";
	else if (MenuNum != 2)
		MenuSelection();
	builder.Clear();
}

void RecvCharRoom(char* buffer)
{
	flatbuffers::FlatBufferBuilder builder;
	auto s2cRoomNty = GetS2C_ROOM_ENTER_NTY(buffer);
	builder.Finish(CreateS2C_ROOM_ENTER_NTY(builder, s2cRoomNty->size(), s2cRoomNty->code(), s2cRoomNty->roomNo(), s2cRoomNty->userIdx() ));

	int roomNo = s2cRoomNty->roomNo() == 0 ?  -1 : s2cRoomNty->roomNo();
	cout << "\n[recv] msg received. 총 버퍼 사이즈 : \"" << s2cRoomNty->size() << "\", Code(채팅 에코:1, 채팅 룸:2, 방 번호 유무:4): \"" << s2cRoomNty->code() << "\", Room No: \"" << roomNo << "\" from server \"" << s2cRoomNty->userIdx() << "\" 번째 Client" << endl;// 받은 숫자를 콘솔 창에 출력
	mainLoop = true;

	if (MenuNum == 1)
		cout << "\n문자를 입력해주세요: (-1 입력 시 종료) ";
	else if (MenuNum != 2)
		MenuSelection();
	builder.Clear();
}

void RecvCharPid(char* buffer)
{
	auto s2cPidAck = GetS2C_PID_ACK(buffer);
	cout << "\n유저 번호: " << s2cPidAck->pid() << endl;
	pid = s2cPidAck->pid();
}

void RecvCharValidRoomNo(char* buffer)
{
	flatbuffers::FlatBufferBuilder builder;
	auto s2cRoomNty = GetS2C_ROOM_ENTER_NTY(buffer);

	if (s2cRoomNty->roomNo() == -100)
	{
		cout << "[ACK] 없는 방 입니다. 방을 다시 입력 하세요" << endl;
		mainLoop = true;
	}
	else
	{
		builder.Finish(CreateS2C_ROOM_ENTER_NTY(builder, s2cRoomNty->size(), 2, s2cRoomNty->roomNo(), s2cRoomNty->userIdx())); // 코드 변경
		s2cRoomNty = GetS2C_ROOM_ENTER_NTY(builder.GetBufferPointer());
		int tmpRoomNo = s2cRoomNty->roomNo() == 0 ? -1 : s2cRoomNty->roomNo();
		int sendResult;
		cout << "[ACK] " << tmpRoomNo << "번방 선택!" << endl;

		builder.Clear();
		builder.Finish(CreateC2S_ROOM_ENTER_REQ(builder, builder.GetSize(), s2cRoomNty->code(), s2cRoomNty->roomNo(), s2cRoomNty->userIdx())); // 코드 변경
		char tmpBuffer[BUF_SIZE] = { 0, };
		memcpy(&tmpBuffer, builder.GetBufferPointer(), builder.GetSize());
		hSocket.write_some(boost::asio::buffer(tmpBuffer), error);
		if (error)// 서버에 보내는 것을 실패 했을 때
			sendResult = 0;
		else // 입력 받은 문자를 서버에 보냄
			sendResult = 1;

		builder.Clear();
		builder.Finish(CreateS2C_ROOM_ENTER_ACK(builder, builder.GetSize(), s2cRoomNty->code(), s2cRoomNty->roomNo(), sendResult));
		auto s2cRoomAck = GetS2C_ROOM_ENTER_ACK(builder.GetBufferPointer());
		cout << "[ACK] [send] \"" << "총 버퍼 사이즈: \"" << s2cRoomAck->size() << "\" , Code(채팅 에코:1, 채팅 룸:2, 방 번호 유무:4): \"" << s2cRoomAck->code() << "\" , room No: \"" << tmpRoomNo << "\" Result(채팅 방 입장 성공: 1, 채팅 방 입장 실패: 0): \"" << s2cRoomAck->result() << "\" from " << pid << "번째 client" << endl;
	}
	builder.Clear();
}

void Char_Recv(const boost::system::error_code& ec)//클라이언트에서 문자열 입력을 받는 도중에 서버에서 문자열을 받으면 스레드를 이용해서 밑에 함수 실행함
{
	while (threadStop) // 소켓이 돌고 있으므로 소켓 먼저 종료 시키고 스레드 종료 시키면 정상 종료
	{
		char tmpBuffer[3000] = { 0, };
		hSocket.read_some(boost::asio::buffer(tmpBuffer), error);
		if (error)
		{
			//throw boost::system::system_error(error);
		}
		else
		{ 
			auto s2cPidAck = GetS2C_PID_ACK(tmpBuffer);
			switch (s2cPidAck->code())
			{
			case Code::NO_CHOICE:
				break;
			case Code::CHAT_ECHO:
				callbackMap[CHAT_ECHO](tmpBuffer);
				break;
			case Code::CHAT_ROOM:
				callbackMap[CHAT_ROOM](tmpBuffer);
				break;
			case Code::PID:
				callbackMap[PID](tmpBuffer);
				mainLoop = true;
				break;
			case Code::VALID_ROOM_NO:
				callbackMap[VALID_ROOM_NO](tmpBuffer);
				break;
			}
		}
		memset(tmpBuffer, 0, BUF_SIZE);
	}
}
