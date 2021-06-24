#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>

#include "game.h"
#include "protocol.h"

struct Board board;

using namespace std;

int main( int argc, char *argv[] ){
  FILE *pipe_rx, *fout = fopen("log/bare.log", "w");
  int l, c, i;
  struct ServerMessage sm;
  struct NameTable ntab;

  if( argc < 2 ){
    fprintf(stderr, "usage: %s <input pipe>\n", argv[0]);
    return 1;
  }

  pipe_rx = fopen(argv[1], "rb");

  fread(&ntab, sizeof(struct NameTable), 1, pipe_rx);

  boardInit(&board, 2);// doar ca sa putem accesa board->game_state la prima iteratie

  while( board.game_state == GAME_STATE_PLAYING ){
    protocolClientRX(pipe_rx, &sm);
    board = sm.board;
    fprintf(fout, "--------------------------------------\n");
    fprintf(fout, "%s: current %s (%d)\n", argv[0], ntab.pnames[board.current], board.current);
    for( i = 0 ; i < board.np ; i++ ){
      fprintf(fout, "%s: player %s (%d):\n", argv[0], ntab.pnames[i], i);
      fprintf(fout, "    -> is_in_game: %d\n", board.is_in_game[i]);
      fprintf(fout, "    -> pposl: %d\n", board.pposl[i]);
      fprintf(fout, "    -> pposc: %d\n", board.pposc[i]);
      fprintf(fout, "    -> nfence: %d\n", board.nfence[i]);
      fprintf(fout, "    -> finish: %d\n", board.finish[i]);
    }
    fprintf(fout, "%s: fences:\n", argv[0]);
    for( l = 0 ; l < BOARD_SIZE + 1 ; l++ ){
      fprintf(fout, "    ->");
      for( c = 0 ; c < BOARD_SIZE + 1 ; c++ ){
        fprintf(fout, " %d%d", board.fence[l][c][VERT], board.fence[l][c][ORIZ]);
      }
      fprintf(fout, "\n");
    }
  }
  
  printf("%s: ending...\n", argv[0]);

  fclose(pipe_rx);
  return 0;
}
