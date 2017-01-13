CC = g++
CFLAGS = -c -g -w -Wall -O2 -std=c++11 -fno-stack-protector

EXEC = main
SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

all: EXEC

EXEC: $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXEC) -g -lssl -lcrypto -lboost_system -Lwebsocketpp -pthread -lcurl -ljansson
%.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

clearscreen:
	clear

clean:
	rm -rf core $(OBJECTS)

remake: clean
	 EXEC