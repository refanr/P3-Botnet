all: server.cpp client.cpp
	g++ -std=c++11 server.cpp -o server
	g++ -std=c++11 client.cpp -o client

client: client.cpp
	g++ -std=c++11 client.cpp -o client

server: server.cpp
	g++ -std=c++11 server.cpp -o server