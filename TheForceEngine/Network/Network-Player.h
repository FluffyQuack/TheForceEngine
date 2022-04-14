#pragma once

#define NETWORK_MAXCLIENTS 128
#define NETWORK_MAXPLAYERNAME 50

enum
{
	NETWORKPLAYERSTATE_UNUSED = 0, //Player slot that has never been used
	NETWORKPLAYERSTATE_ACTIVE = 1 << 0, //Player slot that is currently in use by a connected client
	NETWORKPLAYERSTATE_DISCONNECTED = 1 << 1, //Player slot that was used but the client is currently disconnected (this is used to retain data for clients who might reconnect, and we only use this for a new client if there's no other unused slots left) (this value and behaviour is only handled by the host; clients aren't aware of the difference between a remote client connecting and reconnecting)
	NETWORKPLAYERSTATE_ABOUTTOLEAVE = 1 << 2, //Player that has sent a message indicating they're about to leave the server (only the host ever handles this value)
};

struct networkPlayer_s
{
	int state;
	char name[NETWORK_MAXPLAYERNAME];
	unsigned long long networkId; //Unique identifier for this player. This is set when initializing RakNet, and should stay the same value after connecting to a client (I've got code for handling a guid change after connection, though I'm not sure if that's likely to actually ever happen)
	short id; //Another unique identifer. This is set by our netcode and is meant to be used as an identifier for gameplay packets (we need to attach an id to packets relayed between remote clients, and using this identifier then saves 6 bytes per packet). An id of 0 always means the server's player
	unsigned char colourRed;
	unsigned char colourGreen;
	unsigned char colourBlue;
};

extern networkPlayer_s networkPlayers[NETWORK_MAXCLIENTS];

short Network_GetUnusedPlayerId();
void Network_ResetPlayers(bool resetAll, bool resetLocalPlayerId);
networkPlayer_s *Network_FindDisconnectedPlayerByName(char *name);
networkPlayer_s *Network_FindPlayerByNetworkId(unsigned long long networkId);
networkPlayer_s *Network_FindPlayerByPlayerId(short playerId);
int Network_FindPlayerByPlayerId_ReturnIndex(short playerId);
void Network_MarkPlayerAsDisconnected(unsigned long long networkId);
void Network_RemovePlayer(unsigned long long networkId);
bool Network_AddPlayer(unsigned long long networkId, short playerId, char *name, unsigned char red, unsigned char green, unsigned char blue);
void Network_UpdateGameAfterStoppingNetwork();
void Network_WeGotAcceptedIntoServer(int playerId);
void Network_TemporarilySetOurPlayerIdToInvalid();
void Network_NewPlayerConnected(SLNet::RakNetGUID guid);
void Network_PlayerReconnected(SLNet::RakNetGUID guid);
void Network_PlayerLeft(SLNet::RakNetGUID guid);
void Network_PlayerDisconnected(SLNet::RakNetGUID guid);