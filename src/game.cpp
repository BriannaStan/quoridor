#include <stdlib.h>// pentru exit()
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <cstring>
#include "game.h"

// vectori de directie
int dl[NDIR] = { -1, 0, 1,  0 };
int dc[NDIR] = {  0, 1, 0, -1 };

// vectori de directie folositi in boardMove();
int dl2[NDIR] = { 0, 0, 1, 0 };
int dc2[NDIR] = { 0, 1, 0, 0 };

int vh2d[2] = { DOWN, RIGHT };
int d2vh[NDIR] = { VERT, ORIZ, VERT, ORIZ };

static inline int min( int a, int b ){// foloseste in boardIsFence2()
  return a < b ? a : b;
}

void boardInit( struct Board *b, int np ){
  int i, fence = SUM_FENCE / np;

  // informatii generale
  b->left = b->np = np;
  b->current = 0;
  b->game_state = GAME_STATE_PLAYING;

  // bordam tabla:
  //   este tehnic imposibil modul
  //   in care sunt asezate gardurile
  //   dar merge, deci las asa :)
  for( i = 0 ; i < BOARD_SIZE ; i++ ){
    b->fence[i][0][VERT] = b->fence[i][BOARD_SIZE][VERT] =
    b->fence[0][i][ORIZ] = b->fence[BOARD_SIZE][i][ORIZ] = FENCE;
  }
  
  for( i = 0 ; i < np ; i++ ){
    b->nfence[i] = fence;
    b->is_in_game[i] = 1;
  }

  // adaugam informatiile despre jucatori
  switch( np ){
  case 2:
    b->finish[0] = RIGHT;
    b->pposl[0] = MIDDLE;
    b->pposc[0] = 0;

    b->finish[1] = LEFT;
    b->pposl[1] = MIDDLE;
    b->pposc[1] = BOARD_SIZE - 1;
    break;
  case 4:
    b->finish[0] = UP;
    b->pposl[0] = BOARD_SIZE - 1;
    b->pposc[0] = MIDDLE;

    b->finish[1] = RIGHT;
    b->pposl[1] = MIDDLE;
    b->pposc[1] = 0;

    b->finish[2] = DOWN;
    b->pposl[2] = 0;
    b->pposc[2] = MIDDLE;

    b->finish[3] = LEFT;
    b->pposl[3] = MIDDLE;
    b->pposc[3] = BOARD_SIZE - 1;
    break;
  default:
    fprintf(stderr, "boardInit() from game.h called with np = %d\n", np);
    exit(1);
  }
}

void goToNextPlayer( struct Board *b ){
  int i, np = b->np;

  if( b->left <= 0 )
    return;

  i = (b->current + 1) % np;
  while( !(b->is_in_game[i]) )
    i = (i + 1) % np;

  b->current = i;
}

void kick( struct Board *b ){
  if( b->left <= 1 ){
    b->game_state = GAME_STATE_WIN;
    return;
  }
  
  b->is_in_game[b->current] = 0;
  b->left--;
  goToNextPlayer(b);

  if( b->left <= 1 )
    b->game_state = GAME_STATE_WIN;
}

int boardMove( struct Board *b, int dir ){
  int l, c, f, l1, c1, l2, c2;
  
	if( dir < 0 || dir >= NDIR )// directie valida?
		return 0;

	if( boardIsFence(b, (b->pposl[b->current]) + dl2[dir], (b->pposc[b->current]) + dc2[dir], 1 - d2vh[dir]) )// vedem daca nu este un gard
		return 0;

  // avansam
  l = b->pposl[b->current] += dl[dir];
  c = b->pposc[b->current] += dc[dir];
  f = b->finish[b->current];

  l1 = (BOARD_SIZE - 1) * dl2[f];
  c1 = (BOARD_SIZE - 1) * dc2[f];
  l2 = (BOARD_SIZE - 1) * (dl2[f] + dl[vh2d[1 - d2vh[f]]]);
  c2 = (BOARD_SIZE - 1) * (dc2[f] + dc[vh2d[1 - d2vh[f]]]);

  if( l >= l1 && l <= l2 && c >= c1 && c <= c2 )
    b->game_state = GAME_STATE_WIN;
  else// trecem la urmatorul jucator
    goToNextPlayer(b);
  
	return 1;// returnam success
}

int compNumber[BOARD_SIZE + 1][BOARD_SIZE + 1];
int act;

void Fill( struct Board *b, int i, int j ){
  int l, c, dir;
  
	compNumber[i][j] = act;
	for( dir = 0 ; dir < NDIR ; dir++ ){
	  l = i + dl[dir];
		c = j + dc[dir];
		if( l >= 0 && l < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && !compNumber[l][c] && !boardIsFence2(b, i, j, dir) )
			Fill(b, l, c);
	}
}

int boardIsConnected( struct Board b ){// vrem sa se copieze ca o sa modificam
  int l, c, i, j, minim, ok, vreau;
  
  for( i = 0 ; i < BOARD_SIZE ; i++ )
    for( j = 0 ; j < BOARD_SIZE ; j++ )
      compNumber[i][j] = 0;

  act = 0;
  for( i = 0 ; i < BOARD_SIZE ; i++ ){
		for( j = 0 ; j < BOARD_SIZE ; j++ )
			if( !compNumber[i][j] ){
				act++;
				Fill(&b, i, j);
			}
  }
  
  minim = 1;
  for( i = 0 ; i < 4 ; i++ ){
    if( b.is_in_game[i] ){
      vreau = b.finish[i];
      l = b.pposl[i];
      c = b.pposc[i];
      if(vreau == UP){
        for( i = 0 ; i < BOARD_SIZE ; i++ )
          ok = std::max(ok, (int)(compNumber[1][i] == compNumber[l][c]));
      }else if( vreau == DOWN ){
        for( i = 0 ; i < BOARD_SIZE ; i++ )
          ok = std::max(ok, (int)(compNumber[BOARD_SIZE - 1][i] == compNumber[l][c]));
      }else if( vreau == RIGHT ){
        for( i = 0 ; i < BOARD_SIZE ; i++ )
          ok = std::max(ok, (int)(compNumber[i][BOARD_SIZE - 1] == compNumber[l][c]));
      }else if( vreau == LEFT ){
        for( i = 0 ; i < BOARD_SIZE ; i++ )
          ok = std::max(ok, (int)(compNumber[i][1] == compNumber[l][c]));
      }
      
      minim = std::min(minim, ok);
    }
  }
  
  return minim;
}

int boardAddFence( struct Board *b, int l, int c, int d ){
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
  if( !is_blocked ){
		///nu imi place deloc ce fac mai jos :(
		Board cop = *b;
		cop.fence[l][c][d] = FENCE;
    
    if( boardIsConnected(cop) ){
      b->fence[l][c][d] = FENCE;
      goToNextPlayer(b);
      return 1;
		}
  }

  return 0;
}

int move( struct Board *b, int type, int d, int l, int c ){
  if( type == MOVE_MOVE )
    return boardMove(b, d);
  else
    return boardAddFence(b, l, c, d);
}

int boardIsFence( struct Board *b, int l, int c, int d ){
	if( l < 0 || c < 0 || l > BOARD_SIZE || c > BOARD_SIZE )
		return 1;
	return ((b->fence[l][c][d]) || (b->fence[l - dl[vh2d[d]]][c - dc[vh2d[d]]][d]));
}

int boardIsFence2( struct Board *b, int l, int c, int dir ){
	if( l < 0 || c < 0 || l > BOARD_SIZE || c > BOARD_SIZE )
		return 1;
    return boardIsFence(b, l + dl2[dir], c + dc2[dir], 1 - d2vh[dir]) ;
}
