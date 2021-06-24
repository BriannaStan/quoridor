#include <unistd.h>      // usleep()
#include <math.h>        // geometrie = mate
#include <stdio.h>       // printf()
#include <pthread.h>     // pentru multithreading
#include <sys/time.h>    // pentru getTime() in microsecunde

// chestii grafica
#include <GL/glut.h>
#include <GL/freeglut.h>

#include "game.h"        // functii specifice jocului
#include "protocol.h"    // protocolul de comunicare cu serverul
#include "debug_opts.h"  // DEBUG_GL_COORD si DEBUG_GL_EVENT

//typedef std::chrono::high_resolution_clock Clock;

// functie care returneaza timpul in microsecune
static inline long long getTime(){
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000LL + tv.tv_usec;
}

long long last_move;

static inline int min( int a, int b ){
  return a < b ? a : b;
}

// am pus-o sus ca e nevoie in communication
void prelucrareBoard( struct Board *b ){
  int i;
  
  // scoatem bordura
  for( i = 0 ; i < BOARD_SIZE ; i++ ){
    b->fence[i][0][VERT] = b->fence[i][BOARD_SIZE][VERT] =
    b->fence[0][i][ORIZ] = b->fence[BOARD_SIZE][i][ORIZ] = FREE;
  }
}

/*******************************************
 *                                         *
 *            PIPES AND THREADS            *
 *                                         *
 *******************************************/

pthread_mutex_t board_lock, end_lock, name_lock, time_lock;
int finished = 0;// cand se face 1 interfata grafica stie ca comunicarea s-a oprit
struct Board global_board, prev_board;
struct NameTable ntab;

const int VIEW_SLEEP_DURATION = 10000;

void *communication( void *pipef ){
  FILE *pipe = fopen((char *)pipef, "rb");
  int playing = 1;
  ServerMessage sm;

  if( !pipe ){
    fprintf(stderr, "Unable to open pipefile %s\n", (char *)pipef);
    pthread_exit(NULL);
  }

  pthread_mutex_lock(&name_lock);
  fread(&ntab, sizeof(struct NameTable), 1, pipe);
  pthread_mutex_unlock(&name_lock);
  
  while( playing ){
    protocolClientRX(pipe, &sm);
    pthread_mutex_lock(&board_lock);
    pthread_mutex_lock(&time_lock);
    pthread_mutex_lock(&end_lock);
    
    prev_board = global_board;
    global_board = sm.board;
    prelucrareBoard(&global_board);
    playing = (global_board.game_state == GAME_STATE_PLAYING);
    finished = 1 - playing;
    last_move = getTime();
    
    pthread_mutex_unlock(&board_lock);
    pthread_mutex_unlock(&time_lock);
    pthread_mutex_unlock(&end_lock);
  }

  fclose(pipe);
  pthread_exit(NULL);
}

/*******************************************
 *                                         *
 *                  OPENGL                 *
 *                                         *
 *******************************************/

int windowWidth;
int windowHeight;

const float PI = 4 * atan(1);
const int TEXT_HEIGHT = 32;
const int DIST = 8;

void writeText( const char* string ){
  //glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, (const unsigned char*)string);
  glutBitmapString(GLUT_BITMAP_HELVETICA_18, (const unsigned char*)string);
}

void drawRegPoly( float x, float y, float r, int edges ){
  int i;
  float phi;
  
 	glBegin(GL_POLYGON);
  for( i = 0 ; i < edges ; i++ ){
    phi = i * 2.0f * PI / edges;
		glVertex2f(x + r * cos(phi),
               y + r * sin(phi));
  }
	glEnd();
}

void drawLine( float x1, float y1, float x2, float y2 ){
  //glLineWidth(1.0f);
  glBegin(GL_LINES);
  glVertex2f(x1, y1);
  glVertex2f(x2, y2);
	glEnd();
}

void drawCoordinatesSystem(){
  int i, j;
  
  glLoadIdentity();
  glPointSize(8.0f);
  glEnable(GL_POINT_SMOOTH);
  glBegin(GL_POINTS);
  glColor3f(1.0f, 0.0f, 0.0f);
  for( i = -10 ; i <= +10 ; i++ )
    for( j = -10 ; j <= +10 ; j++ )
      glVertex2i(i, j);
  glEnd();
  glDisable(GL_POINT_SMOOTH);
  glLineWidth(1.0f);
  drawLine(-10, 0, 10, 0);
  drawLine(0, -10, 0, 10);
}

// constante pentru drawBoard()

const int   R = 3;// raport (latura casute) / (distanta intre casute)
const int   INT_SIZE = BOARD_SIZE * R + (BOARD_SIZE - 1);
const float REAL_SIZE = 2.0f;
const float DIMMING_FACTOR = 0.8f;

int display_coord[2 * BOARD_SIZE];

float pcol_default[MAX_PLAYERS][3] = {// culorile jucatorilor
  { 0.0f, 0.0f, 1.0f },// albastru
  { 1.0f, 0.0f, 0.0f },// rosu
  { 1.0f, 0.0f, 1.0f },// mov
  { 1.0f, 1.0f, 0.0f },// galben
};


void initRectCoord(){// calculeaza display_coord[]
  int i;

  for( i = 0 ; i < BOARD_SIZE ; i++ ){
    display_coord[2 * i    ] = (R + 1) * i;
    display_coord[2 * i + 1] = (R + 1) * i + R;
  }
}

static inline float colorTransition( float c1, float c2, int transition ){
  return ((c1 * (255 - transition)) + (c2 * (transition))) / 255;
}

void drawBoard(){
  int i, j, p, l, c, d, dim, transition, intensity;
  float coef;
  unsigned int display[BOARD_SIZE * 2 - 1][BOARD_SIZE * 2 - 1];
  char aux[64];
  float x[3], y[3];
  float pcol[MAX_PLAYERS][3];
  long long now = getTime();

  transition = min((255 * (now - last_move)) / 200000, 255);

  dim = windowHeight < windowWidth ? windowHeight : windowWidth;

  glClear(GL_COLOR_BUFFER_BIT);
  glLoadIdentity();
  glColor3f(0.8f, 0.8f, 0.8f);
  glRasterPos2f(-2.0f, 2.0f);
  writeText("\n");

  for( i = 0 ; i < 2 * BOARD_SIZE - 1 ; i++ )
    for( j = 0 ; j < 2 * BOARD_SIZE - 1 ; j++ )
      display[i][j] = 0;

  for( i = 0 ; i < global_board.np ; i++ )
    if( prev_board.is_in_game[i] ){
      if( prev_board.current == i ){// a facut i mutarea noua?
        display[prev_board.pposl[i] * 2][prev_board.pposc[i] * 2] += (((unsigned int)(255 - transition)) << (8 * i));// de unde vine jucatorul
        if( global_board.is_in_game[i] )
          display[global_board.pposl[i] * 2][global_board.pposc[i] * 2] += (((unsigned int)(transition)) << (8 * i));// unde a ajuns
      }else
        display[global_board.pposl[i] * 2][global_board.pposc[i] * 2] += (255U << (8 * i));
    }

  for( i = 0 ; i < global_board.np ; i++ ){
    pcol[i][0] = pcol_default[i][0];
    pcol[i][1] = pcol_default[i][1];
    pcol[i][2] = pcol_default[i][2];
  }
  
  for( l = 0 ; l < BOARD_SIZE + 1 ; l++ ){
    for( c = 0 ; c < BOARD_SIZE + 1 ; c++ ){
      for( d = 0 ; d < 2 ; d++ ){
        if( global_board.fence[l][c][d] == FENCE ){
          intensity = (prev_board.fence[l][c][d] == FREE) ? transition : 255;
          if( d == 0 ){ // vertical
            display[l * 2    ][c * 2 - 1] =
            display[l * 2 + 1][c * 2 - 1] =
            display[l * 2 + 2][c * 2 - 1] = intensity;
          } else { // orizontal
            display[l * 2 - 1][c * 2    ] =
            display[l * 2 - 1][c * 2 + 1] =
            display[l * 2 - 1][c * 2 + 2] = intensity;
          }
        }
      }
    }
  }

  // text stanga-sus
  if( global_board.game_state == GAME_STATE_PLAYING )
    sprintf(aux, "Current player: %s\n", ntab.pnames[global_board.current]);
  else if( global_board.game_state == GAME_STATE_WIN )
    sprintf(aux, "Game outcome: %s wins!\n", ntab.pnames[global_board.current]);

  writeText(aux);
  
  glLineWidth(2.0f);
  for( l = 0 ; l < 2 * BOARD_SIZE - 1 ; l++ ){
    for( c = 0 ; c < 2 * BOARD_SIZE - 1 ; c++ ){
      x[0] = (display_coord[c]     * REAL_SIZE / INT_SIZE) - (REAL_SIZE / 2);
      x[2] = (display_coord[c + 1] * REAL_SIZE / INT_SIZE) - (REAL_SIZE / 2);
      x[1] = (x[0] + x[2]) / 2;
      
      y[0] = (display_coord[l]     * REAL_SIZE / INT_SIZE) - (REAL_SIZE / 2);
      y[2] = (display_coord[l + 1] * REAL_SIZE / INT_SIZE) - (REAL_SIZE / 2);
      y[1] = (y[0] + y[2]) / 2;
      
      if( ((l & 1) | (c & 1)) == 0 ){// ambele coordonate pare => casuta propriu-zisa
        // desenam casuta
        glColor3f(1.0f, 1.0f, 1.0f);
        glRectd(x[0], y[0], x[2], y[2]);

        for( i = 0 ; i < 2 ; i++ ){
          for( j = 0 ; j < 2 ; j++ ){
            p = j * 2 + i;
            if( (intensity = ((display[l][c] >> (8 * p)) & 255)) > 0 ){
              glColor3f(colorTransition(1.0f, pcol[p][0], intensity),
                        colorTransition(1.0f, pcol[p][1], intensity),
                        colorTransition(1.0f, pcol[p][2], intensity));
              glRectd(x[i], y[j], x[i + 1], y[j + 1]);
            }
          }
        }
      }else if( display[l][c] > 0 ){// este gard
        // desenam dreptunghiul
        glColor3f(0.0f, ((float)display[l][c]) / 255, 0.0f);
        glRectd(x[0], y[0], x[2], y[2]);
      }
    }
  }

  for( i = 0 ; i < global_board.np ; i++ )
    if( global_board.is_in_game[i] ){
      d = global_board.finish[i];
      glColor3f(pcol[i][0], pcol[i][1], pcol[i][2]);
      glRectd(REAL_SIZE * (dc2[d] - 0.5f),
              REAL_SIZE * (dl2[d] - 0.5f),
              REAL_SIZE * (dc2[d] + dc[vh2d[1 - d2vh[d]]] + ((float)dc[d]) / INT_SIZE - 0.5f),
              REAL_SIZE * (dl2[d] + dl[vh2d[1 - d2vh[d]]] + ((float)dl[d]) / INT_SIZE - 0.5f));
    }

  for( i = 0 ; i < global_board.np ; i++ ){
    if( prev_board.current != i ){
      pcol[i][0] = DIMMING_FACTOR * pcol_default[i][0];
      pcol[i][1] = DIMMING_FACTOR * pcol_default[i][1];
      pcol[i][2] = DIMMING_FACTOR * pcol_default[i][2];
    }
  }

  // Culori (partea de jos)
  for( i = 0 ; i < global_board.np ; i++ ){
    glColor3f(pcol[i][0], pcol[i][1], pcol[i][2]);
    glRasterPos2f(-2.0f + 4 * i / global_board.np, -2.0f + ((float)(TEXT_HEIGHT + DIST) / dim));
    sprintf(aux, " %s (%d)", ntab.pnames[i], i);
    writeText(aux);
  }

  if( DEBUG_GL_COORD )
    drawCoordinatesSystem();

  glFlush();
  glutSwapBuffers();
}

void onMouseClick( int btn, int state, int x, int y ){
  if( DEBUG_GL_EVENT )
    printf("onMouseClick %d %d %d %d\n", btn, state, x, y);
}

void onWindowResize( int w, int h ){
  if( DEBUG_GL_EVENT )
    printf("onWindowResize %d %d\n", w, h);
  windowWidth = w;
  windowHeight = h;
  
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (w <= h)
    glOrtho(-2.0, 2.0, -2.0 * (GLfloat) h / (GLfloat) w,
      2.0 * (GLfloat) h / (GLfloat) w, -10.0, 10.0);
  else
    glOrtho(-2.0 * (GLfloat) w / (GLfloat) h,
      2.0 * (GLfloat) w / (GLfloat) h, -2.0, 2.0, -10.0, 10.0);
  glMatrixMode(GL_MODELVIEW);
}

void onKeyPressed( unsigned char key, int x, int y ){
  if( DEBUG_GL_EVENT )
    printf("onKeyPressed %c %d %d\n", key, x, y);
}

void display(){
  pthread_mutex_lock(&board_lock);
  pthread_mutex_lock(&name_lock);
  pthread_mutex_lock(&time_lock);
  drawBoard();
  pthread_mutex_unlock(&board_lock);
  pthread_mutex_unlock(&name_lock);
  pthread_mutex_unlock(&time_lock);
}

void nextMove(){
  //auto currentTime = Clock::now();
  //int duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastRenderTime).count();
  
  //if (duration > 1000) {
  //  duration -= 1000;
  //  lastRenderTime += std::chrono::milliseconds(1000);
  //}
  display();
  usleep(VIEW_SLEEP_DURATION);
}

int initGui( int argc, char *argv[] ){
  // calcul constante
  initRectCoord();
  
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize(800, 800);/// Returneaza 1 daca mutarea (de tip - pune un gard) este valida si o efectueaza si 0 daca nu este - type este fie VERT, fie ORIZ
  glutCreateWindow("Quorior - GUI Viewer");
  glutReshapeFunc(onWindowResize);
  glutDisplayFunc(display);
	glutIdleFunc(nextMove);
	glutMouseFunc(onMouseClick);
  glutKeyboardFunc(onKeyPressed);
  glDisable(GL_LIGHTING);
  glutMainLoop();
  return 0;
}

int main( int argc, char *argv[] ){
  pthread_t comm;
  int i;

  if( argc < 2 ){
    fprintf(stderr, "usage: %s <pipefile>\n", argv[0]);
    return 1;
  }

  if( pthread_mutex_init(&board_lock, NULL) ){
    fprintf(stderr, "Unable to create board mutex\n");
    return 1;
  }

  if( pthread_mutex_init(&end_lock, NULL) ){
    fprintf(stderr, "Unable to create end signal mutex\n");
    return 1;
  }

  if( pthread_mutex_init(&name_lock, NULL) ){
    fprintf(stderr, "Unable to create name table mutex\n");
    return 1;
  }
  
  if( pthread_mutex_init(&time_lock, NULL) ){
    fprintf(stderr, "Unable to create time mutex\n");
    return 1;
  }
  
  printf("%s: ready\n", argv[0]);

  // creem o tabla initiala goala (se va schimba la primul mesaj de la server)
  boardInit(&global_board, 4);
  prelucrareBoard(&global_board);
  prev_board = global_board;

  // creem un name table (se va schimba la primul mesaj de la server)
  for( i = 0 ; i < 2 ; i++ ){
    ntab.pnames[i][0] = '0' + i;
    ntab.pnames[i][1] = '\0';
  }
  
  printf("%s: starting comms\n", argv[0]);
  // initiem thread-ul de comunicatie
  pthread_create(&comm, NULL, communication, ((void *)argv[1]));

  printf("%s: starting gui\n", argv[0]);
  // initiem gui-ul
  initGui(argc, argv);
  
  return 0;
}
