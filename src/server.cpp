//#include <chrono>
//#include <initializer_list>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h> // pentru memcpy()

#include "game.h"
#include "protocol.h"

//typedef std::chrono::high_resolution_clock Clock;
//auto lastRenderTime = Clock::now();
//int plastposl, plastposc;

#define ANSI_LIME "\e[38:5:46m"
#define ANSI_RED  "\e[38:5:196m"
#define ANSI_RESET "\e[0m"

// players
char p_pipe_names[MAX_PLAYERS][2][MAX_STR];
FILE *p_pipes[MAX_PLAYERS][2];
char p_names[MAX_PLAYERS][MAX_STR];
char p_bin[MAX_PLAYERS][MAX_STR];

// viewers
char v_pipe_names[MAX_VIEWERS][2][MAX_STR];
FILE *v_pipes[MAX_VIEWERS][2];
char v_names[MAX_VIEWERS][MAX_STR];
char v_bin[MAX_VIEWERS][MAX_STR];

struct Board board;

int main( int argc, char *argv[] ){
  FILE *plist, *vlist;
  int pcount, vcount, i, ret;
  char aux[MAX_STR];
  PlayerMessage pm;
  ServerMessage sm;
  NameTable ntab;

  if( argc < 3 ){
    fprintf(stderr, "usage: %s <player list file> <viewer list file>\n", argv[0]);
    exit(1);
  }

  plist = fopen(argv[1], "r");
  if( !plist ){
    fprintf(stderr, "could not open player list file %s\n", argv[1]);
    exit(1);
  }

  vlist = fopen(argv[2], "r");
  if( !vlist ){
    fprintf(stderr, "could not open viewer list file %s\n", argv[2]);
    exit(1);
  }

  fprintf(stderr, ANSI_LIME "(*)" ANSI_RESET " Successfuly opened info files, parsing now...\n");

  fprintf(stderr, "player list file %s:\n", argv[1]);
  ret = fscanf(plist, "%d ", &pcount);
  fprintf(stderr, " -> %d players\n", pcount);
  for( i = 0 ; i < pcount ; i++ ){
    ret = fscanf(plist, "%s %s ", p_names[i], p_bin[i]);
    fprintf(stderr, " -> detected player %s with binary %s\n", p_names[i], p_bin[i]);
  }
  fclose(plist);

  fprintf(stderr, "viewer list file %s:\n", argv[2]);
  ret = fscanf(vlist, "%d ", &vcount);
  fprintf(stderr, " -> %d viewers\n", vcount);
  for( i = 0 ; i < vcount ; i++ ){
    ret = fscanf(vlist, "%s %s ", v_names[i], v_bin[i]);
    fprintf(stderr, " -> detected viewer %s with binary %s\n", v_names[i], v_bin[i]);
  }
  fclose(vlist);

  fprintf(stderr, ANSI_LIME "(*)" ANSI_RESET " Initializing board...\n");

  boardInit(&board, pcount);

  fprintf(stderr, ANSI_LIME "(*)" ANSI_RESET " Creating pipes...\n");

  fprintf(stderr, "player pipes:\n");
  for( i = 0 ; i < pcount ; i++ ){
    sprintf(p_pipe_names[i][0], "pipes/sv2client_%s", p_names[i]);
    sprintf(p_pipe_names[i][1], "pipes/client2sv_%s", p_names[i]);
    sprintf(aux, "mkfifo %s %s", p_pipe_names[i][0], p_pipe_names[i][1]);
    ret = system(aux);
    fprintf(stderr, " -> created pipes %s and %s\n", p_pipe_names[i][0], p_pipe_names[i][1]);
  }

  fprintf(stderr, "viewer pipes:\n");
  for( i = 0 ; i < vcount ; i++ ){
    sprintf(v_pipe_names[i][0], "pipes/sv2client_%s", v_names[i]);
    sprintf(v_pipe_names[i][1], "pipes/client2sv_%s", v_names[i]);
    sprintf(aux, "mkfifo %s"/*" %s"*/, v_pipe_names[i][0]/*, v_pipe_names[i][1]*/);
    ret = system(aux);
    fprintf(stderr, " -> created pipe %s\n", v_pipe_names[i][0]);
  }

  fprintf(stderr, ANSI_LIME "(*)" ANSI_RESET " Starting clients...\n");

  fprintf(stderr, "players:\n");
  for( i = 0 ; i < pcount ; i++ ){
    sprintf(aux, "%s %s %s &", p_bin[i], p_pipe_names[i][0], p_pipe_names[i][1]);
    ret = system(aux);
    fprintf(stderr, " -> Started player %s\n", p_names[i]);
  }

  fprintf(stderr, "viewers:\n");
  for( i = 0 ; i < vcount ; i++ ){
    sprintf(aux, "%s %s %s &", v_bin[i], v_pipe_names[i][0], v_pipe_names[i][1]);
    ret = system(aux);
    fprintf(stderr, " -> Started viewer %s\n", v_names[i]);
  }

  fprintf(stderr, ANSI_LIME "(*)" ANSI_RESET " Connecting to pipes...\n");

  fprintf(stderr, "player pipes:\n");
  for( i = 0 ; i < pcount ; i++ ){
    p_pipes[i][0] = fopen(p_pipe_names[i][0], "wb");
    p_pipes[i][1] = fopen(p_pipe_names[i][1], "rb");
    fwrite(&i, sizeof(int), 1, p_pipes[i][0]);// zicem jucatorilor cine sunt
    fflush(p_pipes[i][0]);
    fprintf(stderr, " -> connected to pipes %s and %s\n", p_pipe_names[i][0], p_pipe_names[i][1]);
  }

  // building name table
  for( i = 0 ; i < pcount ; i++ )
    memcpy(ntab.pnames[i], p_names[i], sizeof(char) * MAX_STR);

  fprintf(stderr, "viewer pipes:\n");
  for( i = 0 ; i < vcount ; i++ ){
    v_pipes[i][0] = fopen(v_pipe_names[i][0], "wb");
    //v_pipes[i][1] = fopen(v_pipe_names[i][1], "rb");
    fwrite(&ntab, sizeof(struct NameTable), 1, v_pipes[i][0]);// zicem viewerilor numele jucatorilor
    fflush(v_pipes[i][0]);
    fprintf(stderr, " -> connected to pipe %s\n", v_pipe_names[i][0]);
  }
  
  fprintf(stderr, ANSI_LIME "(*)" ANSI_RESET " All set up!\n");

  // trimitem tabla initiala
  sm.board = board;

  // trimitem la playeri
  for( i = 0 ; i < pcount ; i++ )
    protocolServerTX(p_pipes[i][0], &sm);

  // trimitem la vieweri
  for( i = 0 ; i < vcount ; i++ )
    protocolServerTX(v_pipes[i][0], &sm);
  
  printf(ANSI_LIME "(*)" ANSI_RESET " Press enter to start game...");
  fflush(stdout);
  fgetc(stdin);

  fprintf(stderr, ANSI_LIME "(*)" ANSI_RESET " Starting game!\n");

  // bucla principala
  while( board.game_state == GAME_STATE_PLAYING ){
    protocolServerRX(p_pipes[board.current][1], &pm);// asteptam mutarea jucatorului curent
    // efectuam mutarea
    if( !move(&board, pm.type, pm.d, pm.l, pm.c) ){
      fprintf(stderr, ANSI_RED "(*)" ANSI_RESET " %s (%d) made an incorect move... kicking from game\n", p_names[board.current], board.current);
      kick(&board);
    }

    sm.board = board;// creem mesajul pentru clienti

    // trimitem la jucatori
    for( i = 0 ; i < pcount ; i++ )
      protocolServerTX(p_pipes[i][0], &sm);

    // trimitem la vieweri
    for( i = 0 ; i < vcount ; i++ )
      protocolServerTX(v_pipes[i][0], &sm);

    usleep(SERVER_SLEEP_DURATION);
  }
  sm.board = board;// creem mesajul pentru clienti
  
  fprintf(stderr, ANSI_LIME "(*)" ANSI_RESET " Game ended...\n");

  usleep(100000);

  printf(ANSI_LIME "(*)" ANSI_RESET " Press enter to close players and pipes...");
  fflush(stdout);
  fgetc(stdin);

  fprintf(stderr, ANSI_LIME "(*)" ANSI_RESET " Closing Players...\n");

  // metoda urata. am facut asa doar ca sa implementez mai rapid
  for( i = 0 ; i < pcount ; i++ ){
    sprintf(aux, "killall %s", p_bin[i]);
    ret = system(aux);
    fprintf(stderr, " -> Closed player %s\n", p_names[i]);
  }

  fprintf(stderr, ANSI_LIME "(*)" ANSI_RESET " Closing pipes...\n");

  fprintf(stderr, "player pipes:\n");
  for( i = 0 ; i < pcount ; i++ ){
    closeAndDestroyPipe(p_pipe_names[i][0], p_pipes[i][0]);
    closeAndDestroyPipe(p_pipe_names[i][1], p_pipes[i][1]);
    fprintf(stderr, " -> closed pipes %s and %s\n", p_pipe_names[i][0], p_pipe_names[i][1]);
  }

  fprintf(stderr, "viewer pipes:\n");
  for( i = 0 ; i < vcount ; i++ ){
    closeAndDestroyPipe(v_pipe_names[i][0], v_pipes[i][0]);
    //closeAndDestroyPipe(v_pipe_names[i][1], v_pipes[i][1]);
    fprintf(stderr, " -> closed pipe %s\n", v_pipe_names[i][0]);
  }

  fprintf(stderr, ANSI_LIME "(*)" ANSI_RESET " Server ends gracefully\n");
  return 0;
}
