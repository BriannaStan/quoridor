#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>

#include "game.h"

const int MAX_STR = 128;
const int SERVER_SLEEP_DURATION = 500000;// in microsecunde

// the server sends this structure to every viewer before the game starts
struct NameTable {
  char pnames[MAX_PLAYERS][MAX_STR];
};

// client -> server
struct PlayerMessage {
  int type;// MOVE_MOVE sau MOVE_FENCE
  int d;
  int l;
  int c;// l si c sunt folosite doar daca move_type == MOVE_FENCE
};

// server -> client
struct ServerMessage {
  struct Board board;
};

FILE *createAndOpenPipe( char *fname, const char *rw_opts );
void closeAndDestroyPipe( char *fname, FILE *pipe );

void protocolClientRX( FILE *pipe, ServerMessage *m );
void protocolClientTX( FILE *pipe, PlayerMessage *m );
int  protocolServerRX( FILE *pipe, PlayerMessage *m );
int  protocolServerTX( FILE *pipe, ServerMessage *m );

#endif
