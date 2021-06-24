#ifndef GAME_H
#define GAME_H

//#include <bits/stdc++.h>

// constante generale
const int BOARD_SIZE = 9;
const int MAX_PLAYERS = 4;
const int MAX_VIEWERS = 4;
const int NDIR = 4;
const int FENCE_LEN = 2;
const int MIDDLE = (BOARD_SIZE - 1) / 2;
const int SUM_FENCE = 20;

// constante pentru b->fence[][]
const int FREE  = 0;
const int FENCE = 1;

// directii
const int UP          = 0;
const int RIGHT       = 1;
const int DOWN        = 2;
const int LEFT        = 3;
const int PLACE_FENCE = 4;

const int VERT        = 0;
const int ORIZ        = 1;

// game states
const int GAME_STATE_PLAYING = 0;
const int GAME_STATE_WIN     = 1;// b->current este castigatorul

// tipuri de mutari
const int MOVE_MOVE  = 0;
const int MOVE_FENCE = 1;

// vectori de directie
extern int dl[NDIR];
extern int dc[NDIR];

// vectori de directie folositi in boardMove();
extern int dl2[NDIR];
extern int dc2[NDIR];

extern int vh2d[2];
extern int d2vh[NDIR];

struct Board {
  int np;// numarul de jucatori LA INCEPUTUL JOCULUI
  int left;// numarul de jucatori RAMASI
  int current;// jucatorul la mutare
  int game_state;

  int finish[4];// directia corespunzatoare laturii la care termina jucatorul respectiv
  int nfence[4];// numarul de bariere ramase
  int is_in_game[4];// daca jucatorul mai este in joc
  
  int pposl[4];// liniile jucatoriilor (indexat DE LA 0 ca la grafica e mai naspa de la 1)
  int pposc[4];// coloanele jucatoriilor (indexat DE LA 0 din ac motiv)

  int fence[BOARD_SIZE + 1][BOARD_SIZE + 1][2];
  // fence[l][c][d] == FENCE inseamna ca avem un gard de lungime FENCE_LEN care incepe de pe punctul
  //                                     laticial (l, c) si se extinde in direcria d (VERT/ORIZ)
};

/// Initializeaza tabla de joc
void boardInit( struct Board *b, int np );

/// da afara pe player din joc
void kick( struct Board *b );

/// avanseaza b->current pana cand da de un jucator care inca este in joc
/// daca b->current este deja in joc de ignora
void goToNextPlayer( struct Board *b );

/// Returneaza 1 daca mutarea (de tip - misca jucatorul) este valida si o efectueaza si 0 daca nu este
/// DACA MUTAREA ESTE INCORECTA NU SCHIMBMA b->current!!!
int boardMove( struct Board *b, int dir );

/// Returneaza 1 daca mutarea (de tip - pune un gard) este valida si o efectueaza si 0 daca nu este - type este fie vertical, fie orizontal
/// DACA MUTAREA ESTE INCORECTA NU SCHIMBMA b->current!!!
int boardAddFence( struct Board *b, int l, int c, int d );

/// Verifica daca fiecare player poate ajunge unde vrea
int boardIsConnected(struct Board b);

// agregare mutare player boardMove sau boardAddFence
int move( struct Board *b, int type, int d, int l, int c );

/// Returneaza 1 daca este vreun gard DE LUNGIME 1 in directia d (VERT sau ORIZ) incepand din (l, c)
int boardIsFence( struct Board *b, int l, int c, int d );

/// Returneaza 1 daca este vreun gard DE LUNGIME 1 in directia d (DOWN, RIGHT, UP, LEFT) incepand din (l, c)
int boardIsFence2( struct Board *b, int l, int c, int d );

#endif
