all: server.cpp client.cpp
	g++ -std=c++11 server.cpp -pthread -o server
	g++ -std=c++11 client.cpp -pthread -o client

client: client.cpp
	g++ -std=c++11 client.cpp -pthread -o client

server: server.cpp
	g++ -std=c++11 server.cpp -pthread -o server