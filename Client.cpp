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

// �Ź� ������ �ƴ� �ѹ� �����ؼ� ��� ����ϱ� ���� ���� ���� ���� ����
SOCKET ServerSocket;

// ��� �������� ������ ��� ���� ���� Map �ڷ��� ����
map<SOCKET, Player> SessionList;

// �÷��̾� ���� �� �۵������� ������ ����
int MyPlayerID = 0;
bool bIsRunnging = true;


// [ ������ ������ ������ ���� ������ �Լ� ]
unsigned WINAPI SendThread(void* Arg)
{
	srand((unsigned int)time(nullptr));

	char Message[1024] = { 0, };
	
	// �÷��̾��� ID, XY ��ǥ�� �������� ������ֱ� ������ Ŭ���̾�Ʈ�� �� ������ ���� �ʱ�ȭ�� �ؼ� �����ϴ�.
	// ���� �α��ο� ���� ó���� ���ؼ� Ÿ���� �������ְ� MakePacket �� ���� ��Ŷ�� ����� �ݴϴ�. 
	PacketManager::PlayerData.ID = 0;
	PacketManager::PlayerData.X = 0;
	PacketManager::PlayerData.Y = 0;
	PacketManager::Type = EPacketType::C2S_Login;
	PacketManager::Size = 12; //Data + Header

	PacketManager::MakePacket(Message);

	// ���� ���������� ������� �α��� ó���� ��Ŷ�� �����մϴ�.
	int SendLength = send(ServerSocket, Message, PacketManager::Size + 4, 0);

	// �α����� ������ ���� Ű �Է¿� ���� ��ǥ�̵� Move ���� ó���Դϴ�.
	while (true)
	{
		// Ű �Է��� ���� ������ ����� ������ Ű �Է� �Լ��� �ɾ��ݴϴ�.
		int KeyCode;
		KeyCode = _getch();
		//std::cin.getline(Message, sizeof(Message));

		// ���� q �� �Է��ϰ� �Ǹ� ������ ���� ���� �۾��� ���ݴϴ�.
		if (KeyCode == 'q')
		{
			// C2S_Logut send
			// ���� �����ϰ� bIsRunnging ������ false �� ���ݴϴ�.
			// �׸��� return �� ���� �Լ��� �����ŵ�ϴ�.
			closesocket(ServerSocket);
			bIsRunnging = false;
			return 0;
		}

		// �̵� ���� ó��, Move
		// ���������� ������ Ŭ���̾�Ʈ�� �̵��ϰ��� �ϴ� ������ ��Ŷ�� ��Ƽ� �����մϴ�.
		// ���⼭�� � Ű�� �Է��ߴ����� ��Ŷ Ÿ���� �������شٰ� ���� �˴ϴ�.
		char SendData[1024] = { 0, };
		PacketManager::PlayerData = SessionList[MyPlayerID];
		PacketManager::PlayerData.X = KeyCode;
		PacketManager::Type = EPacketType::C2S_Move;
		PacketManager::Size = 12;
		PacketManager::MakePacket(SendData);

		// ���������� ������� ��Ŷ�� �����մϴ�.
		int SendLength = send(ServerSocket, SendData, PacketManager::Size + 4, 0);
	}

	return 0;
}


// [ ������ ������Ʈ �� ������ �ޱ� ���� ������ �Լ� ]
unsigned WINAPI RecvThread(void* Arg)
{
	while (true)
	{
		// �⺻������ ��� ������ ���� �޾Ƽ� ó���մϴ�.
		// ���⼭�� MSG_WAITALL �� �̿��� �ݵ�� ��� �����͸� �ް� ó���ϰ� �մϴ�.
		char Header[4] = { 0, };
		int RecvLength = recv(ServerSocket, Header, 4, MSG_WAITALL);
		if (RecvLength <= 0)
		{
			// �ƹ��� ���� ���� �ʾҴٸ� bIsRunnging ������ false �� �մϴ�.
			bIsRunnging = false;
			return 0;
		}

		// ��Ŷ�� ����� ���� ������ Ÿ���� ���� ������ ������ݴϴ�.
		unsigned short DataSize = 0;
		EPacketType PacketType = EPacketType::Max;

		// ��� ����, Disassemble Header
		// �޾ƿ� ����� ������ ��� ���� ������ ����ݴϴ�. �� ������ �̿��ؼ� ��ó�� �۾��� �̾ �̴ϴ�.
		memcpy(&DataSize, &Header[0], 2);
		memcpy(&PacketType, &Header[2], 2);

		// ��Ʈ��ũ ����Ʈ ������ ȣ��Ʈ�� ����Ʈ ������ ��ȯ�ϴ� �۾��� �մϴ�.(ntohs)
		// Server �ڵ�� �ߺ��Ǵ� ������̱⿡ �ڼ��� ���� ���� �ڵ带 �ٽ� Ȯ���غ��ô�.
		DataSize = ntohs(DataSize);
		PacketType = (EPacketType)(ntohs((u_short)PacketType));

		char Data[1024] = { 0, };

		// ������ ó��, Data
		// ���� ������ �Ʒ�ó�� ��� Data �� �ޱ� ���ؼ� MSG_WAITALL �� ���� �� ���������� ������ ����մϴ�.
		RecvLength = recv(ServerSocket, Data, DataSize, MSG_WAITALL);
		if (RecvLength <= 0)
		{
			// 0 ���ϸ� bIsRunnging �� false �� ���ݴϴ�.
			bIsRunnging = false;
			return 0;
		}
		else if (RecvLength > 0)
		{
			// Packet Type
			// 0 ���� ũ�� ��� �����Ͱ� �Ǿ��� �޾ƿԱ� ������ ó���� �����մϴ�.
			switch (PacketType)
			{
				// [ Login Ÿ�� ���� ó�� ]
				case EPacketType::S2C_Login:
				{
					// ���Ӱ� �÷��̾� ������ ���� ��Ŷ�� ����ݴϴ�.
					// �̴� ������ ������ �ٸ� �÷��̾��� ������ ���ؼ� �ϴ� �۾��Դϴ�.
					Player NewPlayer;
					memcpy(&NewPlayer.ID, &Data[0], 4);
					memcpy(&NewPlayer.X, &Data[4], 4);
					memcpy(&NewPlayer.Y, &Data[8], 4);

					// ���������� ����Ʈ ������ �����մϴ�.
					NewPlayer.ID = ntohl(NewPlayer.ID);
					NewPlayer.X = ntohl(NewPlayer.X);
					NewPlayer.Y = ntohl(NewPlayer.Y);

					// ���� ������� ������ ���� ����Ʈ ��, �������� ���� ����Ʈ�� ����ݴϴ�.
					SessionList[NewPlayer.ID] = NewPlayer;

					// ó���� ���� ���� ������ ������ ����ݴϴ�.
					MyPlayerID = NewPlayer.ID;

					cout << "Connect Client : " << SessionList.size() << endl;
				}
				break;

				// [ Spawn Ÿ�� ���� ó�� ]
				case EPacketType::S2C_Spawn:
				{
					// �ߺ��� �ڵ忡 ���� ������ �����մϴ�.
					// ������ ������ �ٸ� �÷����� ��ġ(��ǥ)�� �����ϱ� ���� �۾��Դϴ�.
					Player NewPlayer;
					memcpy(&NewPlayer.ID, &Data[0], 4);
					memcpy(&NewPlayer.X, &Data[4], 4);
					memcpy(&NewPlayer.Y, &Data[8], 4);

					NewPlayer.ID = ntohl(NewPlayer.ID);
					NewPlayer.X = ntohl(NewPlayer.X);
					NewPlayer.Y = ntohl(NewPlayer.Y);

					// ���Ӱ� �޾ƿ� ����(�÷��̾�)�� ���� ����Ʈ�� �̹� �����ϴ��� Ȯ���մϴ�.
					// find �Լ��� ���� ã�� ������ SessionList.end() ��, ���� ����Ʈ�� ������ �̴�
					// ã�� ���ߴٴ� ���� �ǹ��մϴ�. �� ��� ���Ӱ� ����(����)�� ���̱⿡ �÷��̾ ���Ǹ���Ʈ�� �߰��մϴ�.
					if (SessionList.find(NewPlayer.ID) == SessionList.end())
					{
						SessionList[NewPlayer.ID] = NewPlayer;
					}

					cout << "Spawn Client" << endl;
				}
				break;

				// [ Logout Ÿ�� ���� ó�� ]
				case EPacketType::S2C_Logout:
				{
					// �̹��� �α׾ƿ� ���� ó���Դϴ�.
					Player DisconnectPlayer;
					memcpy(&DisconnectPlayer.ID, &Data[0], 4);
					memcpy(&DisconnectPlayer.X, &Data[4], 4);
					memcpy(&DisconnectPlayer.Y, &Data[8], 4);

					DisconnectPlayer.ID = ntohl(DisconnectPlayer.ID);
					DisconnectPlayer.X = ntohl(DisconnectPlayer.X);
					DisconnectPlayer.Y = ntohl(DisconnectPlayer.Y);

					// ���޹��� �α׾ƿ� �� ������ ���� ����Ʈ���� �����ݴϴ�.
					SessionList.erase(DisconnectPlayer.ID);

					cout << "disconnect Client" << endl;
				}
				break;

				// [ Move Ÿ�� ���� ó�� ]
				case EPacketType::S2C_Move:
				{
					// ��ǥ �̵��� ���� ó���Դϴ�.
					Player MovePlayer;
					memcpy(&MovePlayer.ID, &Data[0], 4);
					memcpy(&MovePlayer.X, &Data[4], 4);
					memcpy(&MovePlayer.Y, &Data[8], 4);

					MovePlayer.ID = ntohl(MovePlayer.ID);
					MovePlayer.X = ntohl(MovePlayer.X);
					MovePlayer.Y = ntohl(MovePlayer.Y);

					// �ٲ� ��ǥ���� ������Ʈ�ϱ� ���ؼ� ���� ����Ʈ�� �ش� ���� ������ ã�� ������Ʈ �մϴ�.
					SessionList[MovePlayer.ID] = MovePlayer;
				}
				break;
			}

			// ȭ���� �׸��� ���� �۾��Դϴ�.
			system("cls");
			for (const auto& Data : SessionList)
			{
				// �ݺ����� ���� ��� �������� ������ ȭ�鿡 �׸��� ���ݴϴ�.
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
	// ���� �ʱ�ȭ �� �غ� �۾�
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// ���� ����
	ServerSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	// ���� �ּ� ���� �� �ʱ�ȭ�� ����
	SOCKADDR_IN ServerSockAddr = { 0 , };
	ServerSockAddr.sin_family = AF_INET;
	ServerSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	ServerSockAddr.sin_port = htons(22222);

	// ���� �۾�
	connect(ServerSocket, (SOCKADDR*)&ServerSockAddr, sizeof(ServerSockAddr));

	// �����带 ���� �ڵ� ����
	HANDLE ThreadHandles[2];

	// ������ �Լ� ��� �� ȣ��
	ThreadHandles[0] = (HANDLE)_beginthreadex(0, 0, SendThread, 0, 0, 0);
	ThreadHandles[1] = (HANDLE)_beginthreadex(0, 0, RecvThread, 0, 0, 0);

	// bIsRunnging �� false �� �ƴ� �̻� ���� �ݺ�
	while (bIsRunnging)
	{
		Sleep(1);
	}

	closesocket(ServerSocket);

	WSACleanup();

	return 0;
}