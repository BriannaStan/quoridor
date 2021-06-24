///Asta e mai rautacios, nu-i lasa pe ceilalti sa mearga bine
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

#include "game.h"
#include "protocol.h"

struct Board board;

using namespace std;

int GardBun( struct Board *b, int l, int c, int d ){
  int is_blocked, l1, c1;

  // verificam daca mai are garduri :))))
  if( b->nfence[b->current] <= 0 )
    return 0;

  // verificam sa nu iasa primul capat al gardului din matrice
  if( l < 0 || c < 0 || l > BOARD_SIZE || c > BOARD_SIZE )
    return 0;

  // verificam sa nu iasa al doilea capat al gardului din matrice
  if( l + FENCE_LEN * dl[vh2d[d]] > BOARD_SIZE || c + FENCE_LEN * dc[vh2d[d]] > BOARD_SIZE )
    return 0;

  is_blocked = boardIsFence(b, l, c, d) || boardIsFence(b, l + dl[vh2d[d]], c + dc[vh2d[d]], d);

  l1 = l + dl[vh2d[d]] - dl[vh2d[1 - d]];
  c1 = c + dc[vh2d[d]] - dc[vh2d[1 - d]];

  if( l1 >= 0 && l1 <= BOARD_SIZE && c1 >= 0 && c1 <= BOARD_SIZE )
    is_blocked |= (b->fence[l1][c1][1 - d]);
  if( !is_blocked){
		Board cop;
		///nu imi place deloc ce fac mai jos :(
		for(int i = 0; i <= BOARD_SIZE; i++)
			for(int j = 0; j <= BOARD_SIZE; j++)
				for(int k = 0; k < 2; k++)
					cop.fence[i][j][k] = b->fence[i][j][k];
		for(int i = 0; i < 4; i++){
			cop.pposl[i] = b->pposl[i];
			cop.pposc[i] = b->pposc[i];
			cop.finish[i] = b->finish[i];
			cop.is_in_game[i] = b->is_in_game[i];
		}
		///
		cop.fence[l][c][d] = FENCE;
		int is_connected = boardIsConnected(cop);	
		if(is_connected){
			return 1;
		}
  }

  return 0;
}

int main( int argc, char *argv[] ){
  FILE *pipe_rx, *pipe_tx;
  int me, i, oke, l, c, ll, cc, dir, vrem, vrea, punemGard;
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

  while( board.game_state == GAME_STATE_PLAYING ){
    protocolClientRX(pipe_rx, &sm);
    board = sm.board;
    
    if( board.current == me ){
      l = board.pposl[me];
      c = board.pposc[me];
      vrem = board.finish[me];/// directia unde vrea sa ajunga;
      punemGard = 1; /// Initial pleaca cu ideea ca pune gard;
      if( punemGard){
        pm.type = MOVE_FENCE;
        oke = 1;
        for(i = 0; i < board.np; i++){
          if(i != me){
            ll = board.pposl[i];
            cc = board.pposc[i];
            vrea = board.finish[i];
            if( !boardIsFence2(&board, ll, cc, vrea) ){
              if( vrea == UP ){
                pm.d = ORIZ;
                pm.l = ll;
                pm.c = cc;
                /// de adaugat sa vedem daca e ok
              }else if( vrea == DOWN ){
                pm.d = ORIZ;
                pm.l = ll + 1;
                pm.c = cc;
                /// de adaugat sa vedem daca e ok
              }else if( vrea == RIGHT ){
                pm.d = VERT;
                pm.l = ll;
                pm.c = cc + 1;
                /// de adaugat sa vedem daca e ok
              }else if( vrea == LEFT ){
                pm.d = VERT;
                pm.l = ll;
                pm.c = cc;
                /// de adaugat sa vedem daca e ok
              }
              oke = 0;
              break; ///yes yes, ik
            }
          }
        }
        if( oke || !GardBun(&board, pm.l, pm.c, pm.d) )
          punemGard = 0;
      }
      if( !punemGard ){
        pm.type = MOVE_MOVE;
        if( !boardIsFence2(&board, l, c, vrem) )
          pm.d = vrem;
        else{
          do
            dir = rand() % 4;
          while(boardIsFence2(&board, l, c, dir));
          pm.d = dir;
        }
        
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
