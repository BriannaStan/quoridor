#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

#include "game.h"
#include "protocol.h"

using namespace std;

struct Board board;

const int LIMIT = 20;

int main( int argc, char *argv[] ){
  FILE *pipe_rx, *pipe_tx;
  int me;
  struct ServerMessage sm;
  struct PlayerMessage pm;

  if( argc < 3 ){
    fprintf(stderr, "usage: %s <input pipe> <output pipe>\n", argv[0]);
    return 1;
  }

  pipe_rx = fopen(argv[1], "rb");
  pipe_tx = fopen(argv[2], "wb");

  fread(&me, sizeof(int), 1, pipe_rx);// dap, no kidding

  printf("%s: i am %d\n", argv[0], me);
  boardInit(&board, 2);// doar ca sa putem accesa board->game_state la prima iteratie
	int last = -1;
  while( board.game_state == GAME_STATE_PLAYING ){
    protocolClientRX(pipe_rx, &sm);
    board = sm.board;
    if( board.current == me ){
       int l = board.pposl[me];
	  int c = board.pposc[me];
	  int vrem = board.finish[me];///directia unde vrea sa ajunga;
      pm.type = 0;// sa se miste mai mult in loc sa se autoblockeze
      if( pm.type == MOVE_MOVE ){// muta pion
        //printf("Muta pion\n");
        
        pm.type = MOVE_MOVE;
		if(!boardIsFence2(&board, l, c, vrem))
		  pm.d = vrem;
		else{
			int dir, ok = 1;
			int cnt = 0;
			do{
				ok = 1;
				dir = rand() % 4;
				if(last == UP && dir == DOWN){
					cnt++;
					ok = 0;
					if(cnt == LIMIT)
						ok = 1;
				}else if(last == DOWN && dir == UP){
					cnt++;
					ok = 0;
					if(cnt == LIMIT)
						ok = 1;
				}else if(last == RIGHT && dir == LEFT){
					cnt++;
					ok = 0;
					if(cnt == LIMIT)
						ok = 1;
				}else if(last == LEFT && dir == RIGHT){
					cnt++;
					ok = 0;
					if(cnt == LIMIT)
						ok = 1;
				}
			}while(boardIsFence2(&board, l, c, dir) || !ok);
			pm.d = dir;
		}// mergem catre linia de finish
		last = pm.d;
      }else{ // plaseaza gard aleator
        pm.l = 1 + rand() % (BOARD_SIZE + 1 - FENCE_LEN);
        pm.c = 1 + rand() % (BOARD_SIZE + 1 - FENCE_LEN);
        pm.d = rand() % 2;
        //printf("Pune gard l:%d c:%d d:%d\n",pm.l,pm.c,pm.d);
      }
      //cerr << me << " a trimis " << pm.type << endl;
      protocolClientTX(pipe_tx, &pm);
    }
  }

  printf("%s: ending...\n", argv[0]);

  fclose(pipe_rx);
  fclose(pipe_tx);
  return 0;
}
