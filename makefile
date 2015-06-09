all: client server

server:
	g++ server.cpp -o server -Wall -pedantic -std=c++11 -D _DEBUG=0 -lboost_serialization -lboost_system -lpthread

client:
	g++ client.cpp -o client -Wall -pedantic -std=c++11 -D GLM_FORCE_RADIANS -D _DEBUG=0 -lboost_serialization -lboost_system -lpthread -lSDL2 -lSDL2_gfx -lSDL2_image

clean: clean_server clean_client

clean_server:
	rm -f server

clean_client:
	rm -f client
