#include "protocol.h"
#include <stdio.h>
#include <signal.h>// pentru SIGPIPE
#include <stdlib.h>// pentru exit() si system()

int sigpipeReceived;

void sigpipeHandler( int parameter ){
  sigpipeReceived = 1;
}

FILE *createAndOpenPipe( char *fname, const char *rw_opts ){
  char cmd[64];
  int ret;

  sprintf(cmd, "mkfifo %s", fname);
  if( (ret = system(cmd)) ){
    fprintf(stderr, "Command '%s' exited with status %d\n", fname, ret);
    exit(1);
  }

  printf("created pipe\n");
  
  return fopen(fname, rw_opts);
}

void closeAndDestroyPipe( char *fname, FILE *pipe ){
  char cmd[64];
  int ret;

  sprintf(cmd, "rm %s", fname);
  if( (ret = system(cmd)) ){
    fprintf(stderr, "Command '%s' exited with status %d\n", fname, ret);
    exit(1);
  }

  fclose(pipe);
}

void protocolClientTX( FILE *pipe, PlayerMessage *m ){
  sigpipeReceived = 0;
  signal(SIGPIPE, sigpipeHandler);
  fwrite(m, sizeof(PlayerMessage), 1, pipe);
  fflush(pipe);
  if( sigpipeReceived ){
    fprintf(stderr, "Server failure in potocolClientTX!!!\n");
    exit(0);
  }
}

int protocolServerRX( FILE *pipe, PlayerMessage *m ){
  if( fread(m, sizeof(PlayerMessage), 1, pipe) != 1 )
    return 1;
  else
    return 0;
}

int protocolServerTX( FILE *pipe, ServerMessage *m ){
  sigpipeReceived = 0;
  signal(SIGPIPE, sigpipeHandler);
  fwrite(m, sizeof(ServerMessage), 1, pipe);
  fflush(pipe);
  return sigpipeReceived;
}

void protocolClientRX( FILE *pipe, ServerMessage *m ){
  if( fread(m, sizeof(ServerMessage), 1, pipe) != 1 ){
    fprintf(stderr, "Server failure in potocolClientRX!!!\n");
    exit(0);
  }
}
