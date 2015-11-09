CC = g++
CFLAGS = -Wall -pedantic -std=c++11

all: client server

server:
	$(CC) server.cpp -o server $(CFLAGS) -D _DEBUG=0 -D _INFO=1 -lboost_serialization -lboost_system -lpthread

client:
	$(CC) client.cpp -o client $(CFLAGS) -D _DEBUG=0 -D _INFO=1 -D GLM_FORCE_RADIANS -lboost_serialization -lboost_system -lpthread -lGL -lGLEW -lSDL2 -lGLU -lSDL2_gfx -lSDL2_image

clean: clean_server clean_client

clean_server:
	rm -f server

clean_client:
	rm -f client
