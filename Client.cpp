#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include <iostream>
#include <map>
#include <conio.h>

#include "Packet.h"

#pragma comment(lib, "ws2_32")

using namespace std;

// 매번 생성이 아닌 한번 생성해서 계속 사용하기 위한 전방 소켓 전방 선언
SOCKET ServerSocket;

// 모든 접속중인 유저를 담기 위한 소켓 Map 자료형 선언
map<SOCKET, Player> SessionList;

// 플레이어 정보 및 작동중인지 검증용 변수
int MyPlayerID = 0;
bool bIsRunnging = true;


// [ 서버로 정보를 보내기 위한 스레딩 함수 ]
unsigned WINAPI SendThread(void* Arg)
{
	srand((unsigned int)time(nullptr));

	char Message[1024] = { 0, };
	
	// 플레이어의 ID, XY 좌표는 서버에서 만들어주기 때문에 클라이언트는 값 전달을 위한 초기화만 해서 보냅니다.
	// 이후 로그인에 대한 처리를 위해서 타입을 지정해주고 MakePacket 를 통해 패킷을 만들어 줍니다. 
	PacketManager::PlayerData.ID = 0;
	PacketManager::PlayerData.X = 0;
	PacketManager::PlayerData.Y = 0;
	PacketManager::Type = EPacketType::C2S_Login;
	PacketManager::Size = 12; //Data + Header

	PacketManager::MakePacket(Message);

	// 이제 최종적으로 만들어진 로그인 처리용 패킷을 전달합니다.
	int SendLength = send(ServerSocket, Message, PacketManager::Size + 4, 0);

	// 로그인을 했으면 이제 키 입력에 따른 좌표이동 Move 관련 처리입니다.
	while (true)
	{
		// 키 입력을 담을 변수를 만들고 변수에 키 입력 함수를 걸어줍니다.
		int KeyCode;
		KeyCode = _getch();
		//std::cin.getline(Message, sizeof(Message));

		// 만약 q 를 입력하게 되면 연결을 끊기 위한 작업을 해줍니다.
		if (KeyCode == 'q')
		{
			// C2S_Logut send
			// 소켓 종료하고 bIsRunnging 변수를 false 로 해줍니다.
			// 그리고 return 을 통해 함수를 종료시킵니다.
			closesocket(ServerSocket);
			bIsRunnging = false;
			return 0;
		}

		// 이동 관련 처리, Move
		// 마찬가지로 서버에 클라이언트가 이동하고자 하는 정보를 패킷에 담아서 전달합니다.
		// 여기서는 어떤 키를 입력했는지와 패킷 타입을 지정해준다고 보면 됩니다.
		char SendData[1024] = { 0, };
		PacketManager::PlayerData = SessionList[MyPlayerID];
		PacketManager::PlayerData.X = KeyCode;
		PacketManager::Type = EPacketType::C2S_Move;
		PacketManager::Size = 12;
		PacketManager::MakePacket(SendData);

		// 최종적으로 만들어진 패킷을 전달합니다.
		int SendLength = send(ServerSocket, SendData, PacketManager::Size + 4, 0);
	}

	return 0;
}


// [ 서버로 업데이트 된 정보를 받기 위한 스레딩 함수 ]
unsigned WINAPI RecvThread(void* Arg)
{
	while (true)
	{
		// 기본적으로 헤더 정보를 먼저 받아서 처리합니다.
		// 여기서도 MSG_WAITALL 를 이용해 반드시 모든 데이터를 받고 처리하게 합니다.
		char Header[4] = { 0, };
		int RecvLength = recv(ServerSocket, Header, 4, MSG_WAITALL);
		if (RecvLength <= 0)
		{
			// 아무런 값을 받지 않았다면 bIsRunnging 변수를 false 로 합니다.
			bIsRunnging = false;
			return 0;
		}

		// 패킷의 사이즈를 담을 변수와 타입의 대한 변수도 만들어줍니다.
		unsigned short DataSize = 0;
		EPacketType PacketType = EPacketType::Max;

		// 헤더 분해, Disassemble Header
		// 받아온 헤더의 정보를 방금 만든 변수에 담아줍니다. 이 변수를 이용해서 후처리 작업을 이어갈 겁니다.
		memcpy(&DataSize, &Header[0], 2);
		memcpy(&PacketType, &Header[2], 2);

		// 네트워크 바이트 순서를 호스트의 바이트 순서로 변환하는 작업을 합니다.(ntohs)
		// Server 코드와 중복되는 내용들이기에 자세한 것은 서버 코드를 다시 확인해봅시다.
		DataSize = ntohs(DataSize);
		PacketType = (EPacketType)(ntohs((u_short)PacketType));

		char Data[1024] = { 0, };

		// 데이터 처리, Data
		// 마찬 가지로 아래처럼 모든 Data 를 받기 위해서 MSG_WAITALL 를 통해 다 받을때가지 무한정 대기합니다.
		RecvLength = recv(ServerSocket, Data, DataSize, MSG_WAITALL);
		if (RecvLength <= 0)
		{
			// 0 이하면 bIsRunnging 를 false 로 해줍니다.
			bIsRunnging = false;
			return 0;
		}
		else if (RecvLength > 0)
		{
			// Packet Type
			// 0 보다 크면 어떠한 데이터가 되었든 받아왔기 때문에 처리를 시작합니다.
			switch (PacketType)
			{
				// [ Login 타입 관련 처리 ]
				case EPacketType::S2C_Login:
				{
					// 새롭게 플레이어 정보를 만들어서 패킷에 담아줍니다.
					// 이는 서버에 접속한 다른 플레이어의 정보를 위해서 하는 작업입니다.
					Player NewPlayer;
					memcpy(&NewPlayer.ID, &Data[0], 4);
					memcpy(&NewPlayer.X, &Data[4], 4);
					memcpy(&NewPlayer.Y, &Data[8], 4);

					// 마찬가지로 바이트 순서를 정렬합니다.
					NewPlayer.ID = ntohl(NewPlayer.ID);
					NewPlayer.X = ntohl(NewPlayer.X);
					NewPlayer.Y = ntohl(NewPlayer.Y);

					// 이후 만들어진 정보를 세션 리스트 즉, 접속중인 유저 리스트에 담아줍니다.
					SessionList[NewPlayer.ID] = NewPlayer;

					// 처리를 위한 전역 변수에 정보를 담아줍니다.
					MyPlayerID = NewPlayer.ID;

					cout << "Connect Client : " << SessionList.size() << endl;
				}
				break;

				// [ Spawn 타입 관련 처리 ]
				case EPacketType::S2C_Spawn:
				{
					// 중복된 코드에 대한 설명은 생략합니다.
					// 서버에 접속한 다른 플레어의 위치(좌표)를 생성하기 위한 작업입니다.
					Player NewPlayer;
					memcpy(&NewPlayer.ID, &Data[0], 4);
					memcpy(&NewPlayer.X, &Data[4], 4);
					memcpy(&NewPlayer.Y, &Data[8], 4);

					NewPlayer.ID = ntohl(NewPlayer.ID);
					NewPlayer.X = ntohl(NewPlayer.X);
					NewPlayer.Y = ntohl(NewPlayer.Y);

					// 새롭게 받아온 정보(플레이어)가 세션 리스트에 이미 존재하는지 확인합니다.
					// find 함수를 통해 찾은 정보가 SessionList.end() 즉, 세션 리스트의 끝으로 이는
					// 찾지 못했다는 것을 의미합니다. 이 경우 새롭게 생성(접속)된 것이기에 플레이어를 세션리스트에 추가합니다.
					if (SessionList.find(NewPlayer.ID) == SessionList.end())
					{
						SessionList[NewPlayer.ID] = NewPlayer;
					}

					cout << "Spawn Client" << endl;
				}
				break;

				// [ Logout 타입 관련 처리 ]
				case EPacketType::S2C_Logout:
				{
					// 이번엔 로그아웃 관련 처리입니다.
					Player DisconnectPlayer;
					memcpy(&DisconnectPlayer.ID, &Data[0], 4);
					memcpy(&DisconnectPlayer.X, &Data[4], 4);
					memcpy(&DisconnectPlayer.Y, &Data[8], 4);

					DisconnectPlayer.ID = ntohl(DisconnectPlayer.ID);
					DisconnectPlayer.X = ntohl(DisconnectPlayer.X);
					DisconnectPlayer.Y = ntohl(DisconnectPlayer.Y);

					// 전달받은 로그아웃 할 유저를 세션 리스트에서 지워줍니다.
					SessionList.erase(DisconnectPlayer.ID);

					cout << "disconnect Client" << endl;
				}
				break;

				// [ Move 타입 관련 처리 ]
				case EPacketType::S2C_Move:
				{
					// 좌표 이동에 관한 처리입니다.
					Player MovePlayer;
					memcpy(&MovePlayer.ID, &Data[0], 4);
					memcpy(&MovePlayer.X, &Data[4], 4);
					memcpy(&MovePlayer.Y, &Data[8], 4);

					MovePlayer.ID = ntohl(MovePlayer.ID);
					MovePlayer.X = ntohl(MovePlayer.X);
					MovePlayer.Y = ntohl(MovePlayer.Y);

					// 바뀐 좌표값을 업데이트하기 위해서 세션 리스트에 해당 유저 정보를 찾아 업데이트 합니다.
					SessionList[MovePlayer.ID] = MovePlayer;
				}
				break;
			}

			// 화면을 그리기 위한 작업입니다.
			system("cls");
			for (const auto& Data : SessionList)
			{
				// 반복문을 통해 모든 유저들의 정보를 화면에 그리게 해줍니다.
				COORD Cur;
				Cur.X = Data.second.X;
				Cur.Y = Data.second.Y;
				SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Cur);

				cout << Data.second.ID << endl;
			}
		}
	}

	return 0;
}

int main()
{
	// 서버 초기화 및 준비 작업
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// 소켓 생성
	ServerSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	// 소켓 주소 생성 및 초기화와 설정
	SOCKADDR_IN ServerSockAddr = { 0 , };
	ServerSockAddr.sin_family = AF_INET;
	ServerSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	ServerSockAddr.sin_port = htons(22222);

	// 연결 작업
	connect(ServerSocket, (SOCKADDR*)&ServerSockAddr, sizeof(ServerSockAddr));

	// 스레드를 위한 핸들 생성
	HANDLE ThreadHandles[2];

	// 스레드 함수 등록 및 호출
	ThreadHandles[0] = (HANDLE)_beginthreadex(0, 0, SendThread, 0, 0, 0);
	ThreadHandles[1] = (HANDLE)_beginthreadex(0, 0, RecvThread, 0, 0, 0);

	// bIsRunnging 이 false 가 아닌 이상 무한 반복
	while (bIsRunnging)
	{
		Sleep(1);
	}

	closesocket(ServerSocket);

	WSACleanup();

	return 0;
}