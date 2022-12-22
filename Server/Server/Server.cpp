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
	shared_ptr<boost::asio::ip::tcp::socket> sock; // 소켓은 프로토콜 정보밖에 없는 객체 
	boost::asio::ip::tcp::endpoint ep;
	int userIndex;
	int bufferSize; // 총 버퍼 길이
	int inputLegnth; // buffer에 들어온 길이;
	int roomNo = 0;

	char buffer[3000] = { 0, };
};

// 콜백 함수를 핸들러라고 부른다 
int userNum = 1;

class Server
{
	boost::asio::io_service ios;
	shared_ptr<boost::asio::io_service::work> work;
	boost::asio::ip::tcp::endpoint ep;
	boost::asio::ip::tcp::acceptor gate;
	std::vector<Session*> sessions;
	boost::thread_group threadGroup;
	boost::mutex lock;
	boost::system::error_code error;

public:
	Server(unsigned short port_num) :
		work(new boost::asio::io_service::work(ios)),
		ep(boost::asio::ip::tcp::v4(), port_num),
		gate(ios, ep.protocol())
	{
		roomList.push_back(0); // -1 번방
		roomList.push_back(1);
		roomList.push_back(2);
	}

	void Start()
	{
		cout << "Start Server" << endl;
		cout << "Creating Threads" << endl;
		for (int i = 0; i < BUF_SIZE; i++) // 여기 개수 만큼 유저 추가 가능
			threadGroup.create_thread(bind(&Server::WorkerThread, this));

		// thread 잘 만들어질때까지 잠시 기다리는 부분
		this_thread::sleep_for(chrono::milliseconds(100));
		cout << "Threads Created" << endl;

		ios.post(bind(&Server::OpenGate, this));

		threadGroup.join_all();
	}

private:
	void WorkerThread()
	{
		ios.run();
	}

	void OpenGate()
	{
		boost::system::error_code ec;
		gate.bind(ep, ec);
		if (ec)
		{
			cout << "bind failed: " << ec.message() << endl;
			return;
		}

		gate.listen();
		cout << "Gate Opened" << endl;

		StartAccept();
		cout << "[" << boost::this_thread::get_id() << "]" << " Start Accepting" << endl;
	}

	// 비동기식 Accept
	void StartAccept()
	{
		Session* session = new Session();
		shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(ios));
		session->sock = sock;
		session->userIndex = userNum++;
		session->roomNo = 0; 
		gate.async_accept(*sock, session->ep, bind(&Server::OnAccept, this, _1, session));
	}

	void OnAccept(const boost::system::error_code& ec, Session* session)
	{
		flatbuffers::FlatBufferBuilder builder;

		if (ec)
		{
			cout << "accept failed: " << ec.message() << endl;
			return;
		}

		lock.lock();
		sessions.push_back(session);
		cout << "[" << boost::this_thread::get_id() << "]" << " Client Accepted" << endl;
		lock.unlock();

		ios.post(bind(&Server::Receive, this, session));
		StartAccept();

		osf << session->userIndex << " 번 째 클라이언트 접속" << endl;
		int code = 3;
		builder.Finish(CreateS2C_PID_ACK(builder, 12, code, session->userIndex));
		char s2cPidAck[BUF_SIZE] = { 0, };
		memcpy(&s2cPidAck, builder.GetBufferPointer(), builder.GetSize());
		session->sock->async_write_some(boost::asio::buffer(s2cPidAck), bind(&Server::OnSend, this, error));
		osf << TimeResult() << " 포트 번호: " << port << " 유저 " << session->userIndex << " 번째 Client" << endl;
		builder.Clear();
	}

	// 동기식 Receive (쓰레드가 각각의 세션을 1:1 담당)
	void Receive(Session* session)
	{
		boost::system::error_code ec;
		size_t size;
		size = session->sock->read_some(boost::asio::buffer(session->buffer, sizeof(session->buffer)), ec);

		if (ec)
		{
			cout << "[" << boost::this_thread::get_id() << "] read failed: " << ec.message() << endl;
			CloseSession(session);
			return;
		}
		else if (size == 0)
		{
			cout << "[" << boost::this_thread::get_id() << "] peer wants to end " << endl;
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


	void OnSend(const boost::system::error_code& ec)
	{
		if (ec)
		{
			cout << "[" << boost::this_thread::get_id() << "] async_write_some failed: " << ec.message() << endl;
			return;
		}
	}

	void CloseSession(Session* session)
	{
		// if session ends, close and erase
		for (int i = 0; i < sessions.size(); i++)
		{
			if (sessions[i]->sock == session->sock)
			{
				lock.lock();
				sessions.erase(sessions.begin() + i);
				osf << TimeResult() << " 포트 번호: " << port << ", 종료" <<  "\" from server \"" << session->userIndex << "\" 번째 Client" << endl;// 받은 숫자를 콘솔 창에 출력

				lock.unlock();
				break;
			}
		}

		session->sock->close();
		delete session;
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
				sessions[k]->sock->async_write_some(boost::asio::buffer(sessions[CurrentUserPid]->buffer), bind(&Server::OnSend, this, _1));
			}
		}

		osf << TimeResult() << " 포트 번호: " << port << ", [send] msg received. 전체 사이즈 : \"" << s2cEchoNty->size() << "\" , Code(채팅 에코:1, 채팅 룸:2, 방 번호 유무:4): \"" << s2cEchoNty->code() << "\" , 문자열 길이: \"" << sessions[CurrentUserPid]->inputLegnth << "\" , 문자열: \"" << s2cEchoNty->msg()->c_str() << "\" from server \"" << sessions[CurrentUserPid]->userIndex << "\" 번째 Client" << endl;// 받은 숫자를 콘솔 창에 출력
		memset(sessions[CurrentUserPid]->buffer, 0, 1024);

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
				sessions[CurrentUserPid]->sock->async_write_some(boost::asio::buffer(session->buffer), bind(&Server::OnSend, this, _1));
			}
		}
		else
		{
			for (int k = 0; k < sessions.size(); k++)// -1(0) defualt 방
			{
				if (sessions[k]->roomNo == sessions[CurrentUserPid]->roomNo)
				{
					sessions[k]->sock->async_write_some(boost::asio::buffer(session->buffer), bind(&Server::OnSend, this, _1));
				}
			}

			osf << TimeResult() << " 포트 번호: " << port << ", [send] msg received. 총 버퍼 사이즈 : \"" << s2cRoomNty->size() << "\" ,Code(채팅 에코:1, 채팅 룸:2, 방 번호 유무:4): \"" << s2cRoomNty->code() << "\" , Result(1: 방 입장 성공, 0: 방 입장 실패): \"" << 1 << "\" , RoomNo: \"" << roomNo << "\" from server \"" << s2cRoomNty->userIdx() << "\" 번째 Client" << endl;// 받은 숫자를 콘솔 창에 출력							
		}
		builder.ReleaseBufferPointer();
		builder.Clear();
	}
};

int main(void)
{
	file.open("C:\\Users\\secrettown\\source\\repos\\Server\\Server\\log.txt", ios_base::out | ios_base::app);
	//file.open(".\\log.txt", ios_base::out | ios_base::app); // 파일 경로(c:\\log.txt)   exe파일 실행 해야함 
	//osf.rdbuf(file.rdbuf()); // 표준 출력 방향을 파일로 전환

	roomList.push_back(0);
	roomList.push_back(1);
	roomList.push_back(2);
	char c;
	ifstream fin("C:\\Users\\secrettown\\source\\repos\\chattingRoom_BoostAsioStrand\\Server\\Server\\port.txt");
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
	Server serv(atoi(port));
	serv.Start();

	//for (int i = 0; i < 4; ++i) {
	//	thread{ [&]() {
	//		io_context.run();
	//	} }.detach(); // detach 스레드가 언제 종료될지 모른다. 
	//} // join -> 스레드가 종료되는 시점에 자원을 반환받는 것이 보장 
	return 0;
}

////https://codeantenna.com/a/pSHoTCGJrj 참조 ==> 오류	LNK1104	'libboost_thread-vc142-mt-gd-x64-1_81.lib' 파일을 열 수 없습니다.







//#include <iostream>
//#include <boost/asio.hpp>
//#include <boost/thread/mutex.hpp>
//#include <boost/thread.hpp>
//#include <boost/bind.hpp>
//#include <boost/shared_ptr.hpp>
//#include <string>
//#include <vector>
//
//using namespace boost;
//using std::cout;
//using std::endl;
//using std::string;
//
//struct Session
//{
//	shared_ptr<asio::ip::tcp::socket> sock;
//	asio::ip::tcp::endpoint ep;
//	string id;
//	int room_no = -1;
//
//	string sbuf;
//	string rbuf;
//	char buf[80];
//};
//
//
//class Server
//{
//	asio::io_service ios;
//	shared_ptr<asio::io_service::work> work;
//	asio::ip::tcp::endpoint ep;
//	asio::ip::tcp::acceptor gate;
//	std::vector<Session*> sessions;
//	boost::thread_group threadGroup;
//	boost::mutex lock;
//	std::vector<int> existingRooms;
//	const int THREAD_SIZE = 4;
//
//	enum Code { INVALID, SET_ID, CREATE_ROOM, SET_ROOM, WHISPER_TO, KICK_ID };
//
//public:
//	Server(string ip_address, unsigned short port_num) :
//		work(new asio::io_service::work(ios)),
//		ep(asio::ip::address::from_string(ip_address), port_num),
//		gate(ios, ep.protocol())
//	{
//		existingRooms.push_back(0);
//	}
//
//	void Start()
//	{
//		cout << "Start Server" << endl;
//		cout << "Creating Threads" << endl;
//		for (int i = 0; i < THREAD_SIZE; i++)
//			threadGroup.create_thread(bind(&Server::WorkerThread, this));
//
//		// thread 잘 만들어질때까지 잠시 기다리는 부분
//		this_thread::sleep_for(chrono::milliseconds(100));
//		cout << "Threads Created" << endl;
//
//		ios.post(bind(&Server::OpenGate, this));
//
//		threadGroup.join_all(); // 여기서 막혀있다  스레드가 종료할 때 까지 대기 
//		cout << "dd" << endl;
//	}
//
//private:
//	void WorkerThread()
//	{
//		lock.lock();
//		cout << "[" << boost::this_thread::get_id() << "]" << " Thread Start" << endl;
//		lock.unlock();
//
//		ios.run();
//
//		lock.lock();
//		cout << "[" << boost::this_thread::get_id() << "]" << " Thread End" << endl;
//		lock.unlock();
//	}
//
//	void OpenGate()
//	{
//		system::error_code ec;
//		gate.bind(ep, ec);
//		if (ec)
//		{
//			cout << "bind failed: " << ec.message() << endl;
//			return;
//		}
//
//		gate.listen();
//		cout << "Gate Opened" << endl;
//
//		StartAccept();
//		cout << "[" << boost::this_thread::get_id() << "]" << " Start Accepting" << endl;
//	}
//
//	// 비동기식 Accept
//	void StartAccept()
//	{
//		Session* session = new Session();
//		shared_ptr<asio::ip::tcp::socket> sock(new asio::ip::tcp::socket(ios));
//		session->sock = sock;
//		gate.async_accept(*sock, session->ep, bind(&Server::OnAccept, this, _1, session));
//	}
//
//	void OnAccept(const system::error_code& ec, Session* session)
//	{
//		if (ec)
//		{
//			cout << "accept failed: " << ec.message() << endl;
//			return;
//		}
//
//		lock.lock();
//		sessions.push_back(session);
//		cout << "[" << boost::this_thread::get_id() << "]" << " Client Accepted" << endl;
//		lock.unlock();
//
//		ios.post(bind(&Server::Receive, this, session));
//		StartAccept();
//	}
//
//	// 동기식 Receive (쓰레드가 각각의 세션을 1:1 담당)
//	void Receive(Session* session)
//	{
//		system::error_code ec;
//		size_t size;
//		size = session->sock->read_some(asio::buffer(session->buf, sizeof(session->buf)), ec);
//
//		if (ec)
//		{
//			cout << "[" << boost::this_thread::get_id() << "] read failed: " << ec.message() << endl;
//			CloseSession(session);
//			return;
//		}
//
//		if (size == 0)
//		{
//			cout << "[" << boost::this_thread::get_id() << "] peer wants to end " << endl;
//			CloseSession(session);
//			return;
//		}
//
//		session->buf[size] = '\0';
//		session->rbuf = session->buf;
//		PacketManager(session);
//		cout << "[" << boost::this_thread::get_id() << "] " << session->rbuf << endl;
//
//		Receive(session);
//	}
//
//	void PacketManager(Session* session)
//	{
//		// :~ 라는 특수(?)메세지를 보내왔을 경우 처리
//		if (session->buf[0] == ':')
//		{
//			Code code = TranslatePacket(session->rbuf);
//
//			switch (code)
//			{
//			case Code::SET_ID:
//				SetID(session);
//				break;
//			case Code::CREATE_ROOM:
//				CreateRoom(session);
//				break;
//			case Code::SET_ROOM:
//				SetRoom(session);
//				break;
//			case Code::WHISPER_TO:
//				WhisperTo(session);
//				break;
//			case Code::KICK_ID:
//				// 미구현
//				break;
//			case Code::INVALID:
//				session->sbuf = "유효하지 않은 명령어 입니다";
//				session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//				break;
//			}
//		}
//		else  // :~ 라는 특수메세지가 아니고 그냥 채팅일 경우
//		{
//			if (session->id.length() != 0) // id length가 0인 경우는 id를 아직 등록하지 않은 경우
//			{
//				string temp = "[" + session->id + "]:" + session->rbuf;
//				SendAll(session, session->room_no, temp, false);
//			}
//			else
//			{
//				session->sbuf = ":set 을 통해 아이디를 먼저 등록하세요";
//				session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//			}
//		}
//	}
//
//	Code TranslatePacket(string message)
//	{
//		string temp = message.substr(0, sizeof(":set ") - 1);
//		// :set 일 경우
//		if (temp.compare(":set ") == 0)
//		{
//			return Code::SET_ID;
//		}
//
//		temp = message.substr(0, sizeof(":createRoom ") - 1);
//		if (temp.compare(":createRoom ") == 0)
//		{
//			return Code::CREATE_ROOM;
//		}
//
//		temp = message.substr(0, sizeof(":setRoom ") - 1);
//		if (temp.compare(":setRoom ") == 0)
//		{
//			return Code::SET_ROOM;
//		}
//
//		temp = message.substr(0, sizeof(":to ") - 1);
//		if (temp.compare(":to ") == 0)
//		{
//			return Code::WHISPER_TO;
//		}
//
//		temp = message.substr(0, sizeof(":kick ") - 1);
//		if (temp.compare(":kick ") == 0)
//		{
//			return Code::KICK_ID;
//		}
//
//		return Code::INVALID;
//	}
//
//
//	void SetID(Session* session)
//	{
//		string temp = session->rbuf.substr(sizeof(":set ") - 1, session->rbuf.length());
//		// 중복된 아이디인지 체크
//		for (int i = 0; i < sessions.size(); i++)
//		{
//			if (temp.compare(sessions[i]->id) == 0)
//			{
//				session->sbuf = "set falied: [" + temp + "]는 이미 사용중인 아이디 입니다";
//				session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//				return;
//			}
//		}
//
//		session->id = temp;
//		session->sbuf = "set [" + temp + "] success!";
//		session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//
//		if (session->room_no == -1)
//		{
//			session->room_no = 0;
//			SendAll(session, 0, "[" + session->id + "] 님이 로비에 입장하였습니다", false);
//		}
//		else
//		{
//			SendAll(session, session->room_no, "[" + session->id + "] 님이 아이디를 변경하였습니다", false);
//		}
//	}
//
//	void CreateRoom(Session* session)
//	{
//		if (session->room_no != 0)
//		{
//			session->sbuf = "creatRoom falied: 방은 로비에서만 생성할 수 있습니다";
//			session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//			return;
//		}
//
//		string temp = session->rbuf.substr(sizeof(":createRoom ") - 1, session->rbuf.length());
//		// 메세지의 방번호 부분이 정수가 맞는지 체크
//		if (IsTheMessageInNumbers(temp) == false)
//		{
//			session->sbuf = "creatRoom falied: [" + temp + "]는 유효하지 않는 값입니다";
//			session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//			return;
//		}
//
//		int num = atoi(temp.c_str());
//		// 이미 존재하는 방인지 체크
//		for (int i = 0; i < existingRooms.size(); i++)
//		{
//			if (existingRooms[i] == num)
//			{
//				session->sbuf = "creatRoom falied: [" + temp + "]번 방은 이미 존재합니다";
//				session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//				return;
//			}
//		}
//
//		lock.lock();
//		existingRooms.push_back(num);
//		lock.unlock();
//
//		session->room_no = num;
//		session->sbuf = "createRoom [" + temp + "] success!";
//		session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//
//		SendAll(session, 0, "[" + temp + "]번 방이 생성되었습니다", false);
//
//		session->sbuf = "[" + temp + "]번 방에 입장하였습니다";
//		session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//	}
//
//	void SetRoom(Session* session)
//	{
//		if (session->id.length() == 0)
//		{
//			session->sbuf = ":set 을 통해 아이디를 먼저 등록하세요";
//			session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//			return;
//		}
//
//		string temp = session->rbuf.substr(sizeof(":setRoom ") - 1, session->rbuf.length());
//		// 메세지의 방번호 부분이 정수가 맞는지 체크
//		if (IsTheMessageInNumbers(temp) == false)
//		{
//			session->sbuf = "setRoom falied: [" + temp + "]는 유효하지 않는 값입니다";
//			session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//			return;
//		}
//
//		int num = atoi(temp.c_str());
//		if (session->room_no == num)
//		{
//			session->sbuf = "setRoom falied: 이미 [" + temp + "]번 방에 있습니다";
//			session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//			return;
//		}
//
//		// 존재하는 방인지 체크
//		for (int i = 0; i < existingRooms.size(); i++)
//		{
//			if (existingRooms[i] == num)
//			{
//				SendAll(session, session->room_no, "[" + session->id + "] 님이 방을 나갔습니다", false);
//				session->room_no = num;
//
//				if (num == 0)
//					session->sbuf = "로비로 이동하였습니다";
//				else
//					session->sbuf = "[" + temp + "]번 방으로 이동하였습니다";
//
//				session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//				return;
//			}
//		}
//
//		session->sbuf = "setRoom falied: [" + temp + "]번 방이 존재하지 않습니다";
//		session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//	}
//
//	void WhisperTo(Session* session)
//	{
//		if (session->id.length() == 0)
//		{
//			session->sbuf = ":set 을 통해 아이디를 먼저 등록하세요";
//			session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//			return;
//		}
//
//		string temp = session->rbuf.substr(sizeof(":to ") - 1, session->rbuf.length());
//		int num = 0;
//		num = temp.find_first_of(' ');
//		if (num == 0)
//		{
//			session->sbuf = "아이디와 메세지 사이 띄워쓰기를 해주세요";
//			session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//			return;
//		}
//
//		string temp2 = temp.substr(0, num);
//		for (int i = 0; i < sessions.size(); i++)
//		{
//			if (sessions[i]->id.compare(temp2) == 0)
//			{
//				sessions[i]->sbuf = "from [" + session->id + "]:" + temp.substr(num + 1, temp.length());
//				sessions[i]->sock->async_write_some(asio::buffer(sessions[i]->sbuf),
//					bind(&Server::OnSend, this, _1));
//				return;
//			}
//		}
//
//		session->sbuf = "아이디를 찾을 수 없습니다";
//		session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//	}
//
//	void SendAll(Session* session, int room_no, string message, bool sendToSenderAsWell)
//	{
//		// 같은 방에 있는 다른 모든 클라이언트들에게 보낸다
//		for (int i = 0; i < sessions.size(); i++)
//		{
//			if ((session->sock != sessions[i]->sock) && (room_no == sessions[i]->room_no))
//			{
//				sessions[i]->sbuf = message;
//				sessions[i]->sock->async_write_some(asio::buffer(sessions[i]->sbuf),
//					bind(&Server::OnSend, this, _1));
//			}
//		}
//
//		// 메세지를 보내온 클라이언트에게도 보낸다
//		if (sendToSenderAsWell)
//		{
//			session->sbuf = message;
//			session->sock->async_write_some(asio::buffer(session->sbuf), bind(&Server::OnSend, this, _1));
//		}
//	}
//
//	void OnSend(const system::error_code& ec)
//	{
//		if (ec)
//		{
//			cout << "[" << boost::this_thread::get_id() << "] async_write_some failed: " << ec.message() << endl;
//			return;
//		}
//	}
//
//	bool IsTheMessageInNumbers(string message)
//	{
//		const char* cTemp = message.c_str();
//
//		// 메세지 내용(방번호)이 정수가 아닐 경우
//		for (int i = 0; i < message.length(); i++)
//		{
//			if (cTemp[i] < '0' || cTemp[i] > '9')
//			{
//				return false;
//			}
//		}
//
//		return true;
//	}
//
//
//	void CloseSession(Session* session)
//	{
//		if (session->room_no != -1)
//		{
//			SendAll(session, 0, "[" + session->id + "]" + "님이 종료하였습니다", false);
//			SendAll(session, session->room_no, "[" + session->id + "]" + "님이 방을 나갔습니다", false);
//		}
//
//		// if session ends, close and erase
//		for (int i = 0; i < sessions.size(); i++)
//		{
//			if (sessions[i]->sock == session->sock)
//			{
//				lock.lock();
//				sessions.erase(sessions.begin() + i);
//				lock.unlock();
//				break;
//			}
//		}
//
//		string temp = session->id;
//		session->sock->close();
//		delete session;
//	}
//};
//
//
//int main()
//{
//	Server serv(asio::ip::address_v4::any().to_string(), 3333);
//	serv.Start();
//
//	return 0;
//}