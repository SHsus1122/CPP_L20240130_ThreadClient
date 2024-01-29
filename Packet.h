#pragma once
#ifndef __PACKET_H__
#define __PACKET_H__

#include <cstring>
#include <Winsock2.h>

enum class EPacketType : unsigned short
{
	C2S_Spawn = 100,
	S2C_Spawn,
	C2S_Login,
	S2C_Login,
	C2S_Logout,
	S2C_Logout,
	C2S_Move,
	S2C_Move,

	Max
};

class Player
{
public:
	int X;
	int Y;
	int ID;
};

class PacketManager
{
public:
	static unsigned short Size;
	static Player PlayerData;
	static EPacketType Type;

	void static MakePacket(char* Buffer)
	{
		//size Type  Id       X			Y      
		//[][] [][] [][][][] [][][][] [][][][] 
		unsigned short Data = htons(Size);
		memcpy(&Buffer[0], &Data, 2);
		Data = htons((unsigned short)(Type));
		memcpy(&Buffer[2], &Data, 2);

		int Data2 = htonl(PlayerData.ID);
		memcpy(&Buffer[4], &Data2, 4);
		Data2 = htonl(PlayerData.X);
		memcpy(&Buffer[8], &Data2, 4);
		Data2 = htonl(PlayerData.Y);
		memcpy(&Buffer[12], &Data2, 4);
	}
};


unsigned short PacketManager::Size = 0;
Player PacketManager::PlayerData = { 0, };
EPacketType PacketManager::Type = EPacketType::C2S_Login;

#endif //__PACKET_H__
