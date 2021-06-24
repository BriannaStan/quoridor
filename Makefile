# compilator C
CC=gcc

# compilator C++
CXX=g++

# -O2 optimizeaza programul (eliminarea recursivitatii la coada, elimina calcule si teste etc.)
# -Wall afiseaza toate warning-urile
CFLAGS=-Wall -O2

# optiuni pentru compilarea programelor grafice
GLFLAGS=-lglut -lGL

# optiuni pentru compilarea cu pthread.h
THREADFLAGS=-lpthread

# optiunea pentru compilare obiect
OBJ_OPT=-c

run: | bin/server bin/gui_view bin/bare_view bin/player_stupid bin/player0 bin/player1 player.list viewers.list pipes log
	bin/server player.list viewers.list

# serverul va porni jucatorii si viewerii (deoarece numarul jucatorilor poate varia)

player.list:
	printf "2\nstupid1 bin/player_stupid\nstupid2 bin/player_stupid\n" > player.list

viewers.list:
	printf "1\ngui bin/gui_view\n" > viewers.list

# compilare

# "librarii" implementate de noi
bin/game.o: src/game.cpp src/game.h | bin
	${CXX} ${CFLAGS} ${OBJ_OPT} src/game.cpp -o bin/game.o

bin/protocol.o: src/protocol.cpp src/protocol.h | bin
	${CXX} ${CFLAGS} ${OBJ_OPT} src/protocol.cpp -o bin/protocol.o

# player prost
bin/player_stupid.o: src/player_stupid.cpp src/game.h src/protocol.h | bin
	${CXX} ${CFLAGS} ${OBJ_OPT} src/player_stupid.cpp -o bin/player_stupid.o

bin/player_stupid: bin/player_stupid.o bin/game.o bin/protocol.o | bin
	${CXX} ${CFLAGS} bin/player_stupid.o bin/game.o bin/protocol.o -o bin/player_stupid

# player 0
bin/player0.o: src/player0.cpp src/game.h src/protocol.h | bin
	${CXX} ${CFLAGS} ${OBJ_OPT} src/player0.cpp -o bin/player0.o

bin/player0: bin/player0.o bin/game.o bin/protocol.o | bin
	${CXX} ${CFLAGS} bin/player0.o bin/game.o bin/protocol.o -o bin/player0

# player 1
bin/player1.o: src/player1.cpp src/game.h src/protocol.h | bin
	${CXX} ${CFLAGS} ${OBJ_OPT} src/player1.cpp -o bin/player1.o

bin/player1: bin/player1.o bin/game.o bin/protocol.o | bin
	${CXX} ${CFLAGS} bin/player1.o bin/game.o bin/protocol.o -o bin/player1

# server
bin/server.o: src/server.cpp src/game.h src/protocol.h | bin
	${CXX} ${CFLAGS} -Wno-format-overflow ${OBJ_OPT} src/server.cpp -o bin/server.o

bin/server: bin/server.o bin/game.o bin/protocol.o | bin
	${CXX} ${CFLAGS} bin/server.o bin/protocol.o bin/game.o -o bin/server


# GUI viewer
bin/gui_view.o: src/gui_view.cpp src/game.h src/protocol.h src/debug_opts.h | bin
	${CXX} ${CFLAGS} ${OBJ_OPT} src/gui_view.cpp -o bin/gui_view.o

bin/gui_view: bin/gui_view.o bin/protocol.o bin/game.o | bin
	${CXX} ${CFLAGS} bin/gui_view.o bin/protocol.o bin/game.o ${GLFLAGS} ${THREADFLAGS} -o bin/gui_view

# BARE viewer (arata doar informatiile tablei direct)
bin/bare_view.o: src/bare_view.cpp src/game.h src/protocol.h src/debug_opts.h | bin
	${CXX} ${CFLAGS} ${OBJ_OPT} src/bare_view.cpp -o bin/bare_view.o

bin/bare_view: bin/bare_view.o bin/protocol.o bin/game.o | bin
	${CXX} ${CFLAGS} bin/bare_view.o bin/protocol.o bin/game.o ${GLFLAGS} ${THREADFLAGS} -o bin/bare_view

bin:
	mkdir bin

pipes:
	mkdir pipes

log:
	mkdir log

clean:
	rm -Rf bin pipes log

kill:
	killall server & wait

install-libs-ubuntu:
	sudo apt install mesa-utils freeglut3-dev
