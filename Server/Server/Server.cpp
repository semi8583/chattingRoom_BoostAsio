#define FD_SETSIZE 1028  // 유저 1027명까지 접속 허용
#include <iostream>
#include <map>
#include <cstdlib>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <string.h>
using boost::asio::ip::tcp;
using namespace std;
#define BUF_SIZE 512

#include <fstream>
#include <string>
#include <string.h>
#include <ctime>
#include "logger.h"
#include <list>

#include "flatbuffers/flatbuffers.h"
#include "C2S_CHATECHO_REQ_generated.h"
#include "C2S_PID_REQ_generated.h"
#include "C2S_ROOM_ENTER_REQ_generated.h"
#include "S2C_CHATECHO_ACK_generated.h"
#include "S2C_CHATECHO_NTY_generated.h"
#include "S2C_PID_ACK_generated.h"
#include "S2C_ROOM_ENTER_ACK_generated.h"
#include "S2C_ROOM_ENTER_NTY_generated.h"

int CurrentUserPid = 0;
CHAR port[10] = { 0, };// = "3587";

ofstream file;
ostreamFork osf(file, cout);

std::vector<int> roomList;

string TimeResult()
{
	time_t timer = time(NULL);// 1970년 1월 1일 0시 0분 0초부터 시작하여 현재까지의 초
	struct tm t;
	localtime_s(&t, &timer); // 포맷팅을 위해 구조체에 넣기
	osf << (t.tm_year + 1900) << "년 " << t.tm_mon + 1 << "월 " << t.tm_mday << "일 " << t.tm_hour << "시 " << t.tm_min << "분 " << t.tm_sec << "초 ";
	return " ";
}

enum Code
{
	NO_CHOICE,
	CHAT_ECHO,
	CHAT_ROOM,
	PID,
	VALID_ROOM_NO
};

struct Session
{
	shared_ptr<boost::asio::ip::tcp::socket> sock;
	boost::asio::ip::tcp::endpoint ep;
	int userIndex;
	int bufferSize; // 총 버퍼 길이
	int inputLegnth; // buffer에 들어온 길이;
	int roomNo = 0;

	char buffer[1024] = { 0, };
};

boost::asio::io_context io_context;
shared_ptr<boost::asio::io_service::work> work(new boost::asio::io_service::work(io_context));
boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), atoi(port));
boost::asio::ip::tcp::acceptor gate(io_context, ep.protocol());
std::vector<Session*> sessions;
boost::thread_group threadGroup;
boost::mutex _lock;

int userNum = 1;

void RecvCharValidRoomNo(Session* session);
void RecvCharEcho(Session* session);
void StartAccept();
void OnAccept(const boost::system::error_code& ec, Session* session);
void OpenGate();
void Receive(Session* session);
void CloseSession(Session* session);
void OnSend(const boost::system::error_code& ec);
void WorkerThread();

void WorkerThread()
{
	_lock.lock();
	osf << "[" << boost::this_thread::get_id() << "]" << " Thread Start" << endl;
	_lock.unlock();

	io_context.run();

	_lock.lock();
	osf << "[" << boost::this_thread::get_id() << "]" << " Thread End" << endl;
	_lock.unlock();
}

void OnSend(const boost::system::error_code& ec)
{
	if (ec)
	{
		osf << "[" << boost::this_thread::get_id() << "] async_write_some failed: " << ec.message() << endl;
		return;
	}
}

void CloseSession(Session* session)
{
	osf << "[" << session->userIndex << "] 님이 종료하였습니다 " << endl;

	// if session ends, close and erase
	for (int i = 0; i < sessions.size(); i++)
	{
		if (sessions[i]->sock == session->sock)
		{
			_lock.lock();
			sessions.erase(sessions.begin() + i);
			_lock.unlock();
			break;
		}
	}

	session->sock->close();
	delete session;
}

void Receive(Session* session)
{
	boost::system::error_code ec;
	size_t size;
	size = session->sock->read_some(boost::asio::buffer(session->buffer, sizeof(session->buffer)), ec);

	if (ec)
	{
		osf << "[" << boost::this_thread::get_id() << "] read failed: " << ec.message() << endl;
		CloseSession(session);
		return;
	}
	else if (size == 0)
	{
		osf << "[" << boost::this_thread::get_id() << "] peer wants to end " << endl;
		CloseSession(session);
		return;
	}
	else
	{
		auto s2cPidAck = GetS2C_PID_ACK(session->buffer);
		int code = 100;
		if (session->buffer[1] == 0 || session->buffer[2] == 0 || session->buffer[3] == 0)
			code = s2cPidAck->code();
		switch (code)
		{
		case Code::CHAT_ECHO:
			RecvCharEcho(session);
			break;
		case Code::VALID_ROOM_NO:
			RecvCharValidRoomNo(session);
			break;
		}
	}

	session->buffer[size] = '\0';

	Receive(session);
}

// 비동기식 Accept
void StartAccept()
{
	flatbuffers::FlatBufferBuilder builder;

	Session* session = new Session();
	shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(io_context));
	session->sock = sock;
	session->roomNo = 0;
	session->userIndex = userNum++;
	gate.async_accept(*sock, session->ep, bind(&OnAccept, _1, session));
	builder.Clear();
}

void OnAccept(const boost::system::error_code& ec, Session* session)
{
	flatbuffers::FlatBufferBuilder builder;

	if (ec)
	{
		osf << "accept failed: " << ec.message() << endl;
		return;
	}

	_lock.lock();
	sessions.push_back(session);
	osf << "[" << boost::this_thread::get_id() << "]" << " Client Accepted" << endl;
	_lock.unlock();

	io_context.post(bind(&Receive, session));
	StartAccept();

	osf << session->userIndex << " 번 째 클라이언트 접속" << endl;
	int code = 3;
	builder.Finish(CreateS2C_PID_ACK(builder, 12, code, session->userIndex));
	char s2cPidAck[BUF_SIZE] = { 0, };
	memcpy(&s2cPidAck, builder.GetBufferPointer(), builder.GetSize());
	session->sock->async_write_some(boost::asio::buffer(s2cPidAck), bind(&OnSend, _1));
	osf << TimeResult() << " 포트 번호: " << port << " 유저 " << session->userIndex << " 번째 Client" << endl;

	builder.Clear();
}

void OpenGate()
{
	boost::system::error_code ec;
	gate.bind(ep, ec);
	if (ec)
	{
		osf << "bind failed: " << ec.message() << endl;
		return;
	}

	gate.listen();
	osf << "Gate Opened" << endl;

	StartAccept();
	osf << "[" << boost::this_thread::get_id() << "]" << " Start Accepting" << endl;
}

int main(void)
{
	flatbuffers::FlatBufferBuilder builder;
	file.open("C:\\Users\\secrettown\\source\\repos\\Server\\Server\\log.txt", ios_base::out | ios_base::app);
	//file.open(".\\log.txt", ios_base::out | ios_base::app); // 파일 경로(c:\\log.txt)   exe파일 실행 해야함 
	//osf.rdbuf(file.rdbuf()); // 표준 출력 방향을 파일로 전환

	roomList.push_back(0);
	roomList.push_back(1);
	roomList.push_back(2);
	char c;
	ifstream fin("C:\\Users\\secrettown\\source\\repos\\Server\\Server\\port.txt");
	//ifstream fin(".\\port.txt");
	if (fin.fail())
	{
		osf << "포트 파일이 없습니다 " << endl;
		return 0;
	}
	int i = 0;
	while (fin.get(c))
	{
		port[i++] = c;
	}
	fin.close(); // 열었던 파일을 닫는다. 
	//osf << "Enter PORT number (3587) :";
	//cin >> port;

	for (int i = 0; i < 4; i++)
		threadGroup.create_thread(bind(&WorkerThread));

	// thread 잘 만들어질때까지 잠시 기다리는 부분
	this_thread::sleep_for(chrono::milliseconds(100));
	osf << "Threads Created" << endl;

	io_context.post(bind(&OpenGate));

	threadGroup.join_all();

	builder.ReleaseBufferPointer();
	builder.Clear();

	return 0;
}

void RecvCharEcho(Session* session)
{
	flatbuffers::FlatBufferBuilder builder;
	auto s2cEchoNty = GetS2C_CHATECHO_NTY(session->buffer);
	builder.Finish(CreateS2C_CHATECHO_NTY(builder, s2cEchoNty->size(), s2cEchoNty->code(), s2cEchoNty->userIdx(), s2cEchoNty->stringLength(), builder.CreateString(s2cEchoNty->msg()->c_str())));

	for (int k = 0; k < sessions.size(); k++)// -1(0) defualt 방
	{
		if (sessions[k]->userIndex == s2cEchoNty->userIdx())
			CurrentUserPid = k;
	}
	memcpy(&sessions[CurrentUserPid]->buffer, builder.GetBufferPointer(), builder.GetSize());

	sessions[CurrentUserPid]->inputLegnth = s2cEchoNty->stringLength();
	sessions[CurrentUserPid]->bufferSize = builder.GetSize();
	sessions[CurrentUserPid]->userIndex = s2cEchoNty->userIdx();

	osf << TimeResult() << " 포트 번호: " << port << ", [recv] msg received. 전체 사이즈 : \"" << s2cEchoNty->size() << "\" , Code(채팅 에코:1, 채팅 룸:2, 방 번호 유무:4): \"" << s2cEchoNty->code() << "\" , 문자열 길이: \"" << sessions[CurrentUserPid]->inputLegnth << "\" , 문자열: \"" << s2cEchoNty->msg()->c_str() << "\" from server \"" << sessions[CurrentUserPid]->userIndex << "\" 번째 Client" << endl;// 받은 숫자를 콘솔 창에 출력

	for (int k = 0; k < sessions.size(); k++)
	{
		if (sessions[k]->roomNo == sessions[CurrentUserPid]->roomNo || sessions[k]->roomNo == -100)
		{
			sessions[CurrentUserPid]->sock->async_write_some(boost::asio::buffer(sessions[CurrentUserPid]->buffer), bind(&OnSend, _1));
		}
	}
	memset(sessions[CurrentUserPid]->buffer, 0, 1024);

	osf << TimeResult() << " 포트 번호: " << port << ", [send] msg received. 전체 사이즈 : \"" << s2cEchoNty->size() << "\" , Code(채팅 에코:1, 채팅 룸:2, 방 번호 유무:4): \"" << s2cEchoNty->code() << "\" , 문자열 길이: \"" << sessions[CurrentUserPid]->inputLegnth << "\" , 문자열: \"" << s2cEchoNty->msg()->c_str() << "\" from server \"" << sessions[CurrentUserPid]->userIndex << "\" 번째 Client" << endl;// 받은 숫자를 콘솔 창에 출력
	builder.Clear();
}

void RecvCharValidRoomNo(Session* session)
{
	flatbuffers::FlatBufferBuilder builder;

	auto s2cRoomNty = GetS2C_ROOM_ENTER_NTY(session->buffer); // c2sreq로 변경 s2cRoomNty.GetMsg() 이거 자체를 넣어버리면 char*가 deserialize에서 초기화 됨 

	builder.Finish(CreateS2C_ROOM_ENTER_NTY(builder, s2cRoomNty->size(), 2, s2cRoomNty->roomNo(), s2cRoomNty->userIdx())); // code 번호 변경
	s2cRoomNty = GetS2C_ROOM_ENTER_NTY(builder.GetBufferPointer());
	for (int k = 0; k < sessions.size(); k++)// -1(0) defualt 방
	{
		if (sessions[k]->userIndex == s2cRoomNty->userIdx())
			CurrentUserPid = k;
	}

	if (find(roomList.begin(), roomList.end(), s2cRoomNty->roomNo()) != roomList.end()) // 서버에 방 번호가 존재 할 시
	{
		sessions[CurrentUserPid]->roomNo = s2cRoomNty->roomNo();
	}
	else // 방 번호가 존재 하지 않을 때  
	{
		sessions[CurrentUserPid]->roomNo = -100;
		builder.Clear();
		builder.Finish(CreateS2C_ROOM_ENTER_NTY(builder, s2cRoomNty->size(), 4, s2cRoomNty->roomNo(), s2cRoomNty->userIdx())); // code 번호 변경
		s2cRoomNty = GetS2C_ROOM_ENTER_NTY(builder.GetBufferPointer());
	}
	builder.Clear();
	builder.Finish(CreateS2C_ROOM_ENTER_NTY(builder, s2cRoomNty->size(), s2cRoomNty->code(), sessions[CurrentUserPid]->roomNo, s2cRoomNty->userIdx()));
	s2cRoomNty = GetS2C_ROOM_ENTER_NTY(builder.GetBufferPointer());

	int roomNo = s2cRoomNty->roomNo() == 0 ? -1 : s2cRoomNty->roomNo();

	sessions[CurrentUserPid]->bufferSize = s2cRoomNty->size();

	builder.Clear();
	builder.Finish(CreateS2C_ROOM_ENTER_NTY(builder, s2cRoomNty->size(), s2cRoomNty->code(), s2cRoomNty->roomNo(), s2cRoomNty->userIdx()));
	memcpy(&sessions[CurrentUserPid]->buffer, builder.GetBufferPointer(), builder.GetSize());

	osf << TimeResult() << " 포트 번호: " << port << ", [recv] msg received. 총 버퍼 사이즈 : \"" << s2cRoomNty->size() << "\" ,Code(채팅 에코:1, 채팅 룸:2, 방 번호 유무:4): \"" << s2cRoomNty->code() << "\" , Result(1: 방 입장 성공, 0: 방 입장 실패): \"" << 1 << "\" , RoomNo: \"" << roomNo << "\" from server \"" << sessions[CurrentUserPid]->userIndex << "\" 번째 Client" << endl;// 받은 숫자를 콘솔 창에 출력

	if (s2cRoomNty->roomNo() == -100)
	{
		builder.Clear();
		builder.Finish(CreateS2C_ROOM_ENTER_NTY(builder, s2cRoomNty->size(), s2cRoomNty->code(), s2cRoomNty->roomNo(), s2cRoomNty->userIdx()));
		char s2cPidAck[BUF_SIZE] = { 0, };
		memcpy(&s2cPidAck, builder.GetBufferPointer(), builder.GetSize());
		for (int l = 0; l < sessions.size(); l++)// -1(0) defualt 방
		{
			sessions[CurrentUserPid]->sock->async_write_some(boost::asio::buffer(session->buffer), bind(&OnSend, _1));
		}
	}
	else
	{
		for (int k = 0; k < sessions.size(); k++)// -1(0) defualt 방
		{
			if (sessions[k]->roomNo == sessions[CurrentUserPid]->roomNo)
			{
				sessions[k]->sock->async_write_some(boost::asio::buffer(session->buffer), bind(&OnSend,  _1));
			}
		}

		osf << TimeResult() << " 포트 번호: " << port << ", [send] msg received. 총 버퍼 사이즈 : \"" << s2cRoomNty->size() << "\" ,Code(채팅 에코:1, 채팅 룸:2, 방 번호 유무:4): \"" << s2cRoomNty->code() << "\" , Result(1: 방 입장 성공, 0: 방 입장 실패): \"" << 1 << "\" , RoomNo: \"" << roomNo << "\" from server \"" << s2cRoomNty->userIdx() << "\" 번째 Client" << endl;// 받은 숫자를 콘솔 창에 출력							
	}
	builder.ReleaseBufferPointer();
	builder.Clear();
}

//https://codeantenna.com/a/pSHoTCGJrj 참조 ==> 오류	LNK1104	'libboost_thread-vc142-mt-gd-x64-1_81.lib' 파일을 열 수 없습니다.