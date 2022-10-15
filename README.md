# P3-Botnet

## Authors
- Reynir Gunnarsson
- Steinar Ísaksson

## Contents

- server.cpp
    - To compile: `g++ -std=c++11 server.cpp`
- client.cpp
    - To compile: `g++ -std=c++11 client.cpp`

## How to run

### server.cpp
`./tsamgroup20 <port_number>`

### client.cpp
`./client <ip port_number>`

## Early deadline submission
We have implemented rudimentary server and client programs. The client is able to connect to the server, and the server accepts commands from the client. 
When the client issues the command `CONNECT,<ip>,<portno>`, the server connects to another server with the given information and sends out a `JOIN,GROUP_20` command to said server, then gets a standard reply: `SERVERS,<group>,<ip>,<port>` which is added to a vector of connections.
When the client issues the command `QUERYSERVERS,`, the server replies with a list of active connections. Querysevers must end with a comma!
