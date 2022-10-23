# P3-Botnet

## Authors
- Reynir Gunnarsson
- Steinar Ísaksson

## Contents

- To compile using Makefile
    - To compile both server.cpp and client.cpp
        - Input `make` in the command line
    - To compile server.cpp
        - Input `make server` in the command line
    - To compile client.cpp
        - Input `make client` in the command line

- server.cpp
    - To compile: `g++ -std=c++11 server.cpp`
    - OS: MacOS
- client.cpp
    - To compile: `g++ -std=c++11 client.cpp`
    - OS: MacOS



## How to run

### server.cpp
`./tsamgroup20 <port_number>`

### client.cpp
`./client <ip port_number>`

## Early deadline submission
We have implemented rudimentary server and client programs. The client is able to connect to the server, and the server accepts commands from the client. 
When the client issues the command `CONNECT,<ip>,<portno>`, the server connects to another server with the given information and sends out a `JOIN,GROUP_20` command to said server, then gets a standard reply: `SERVERS,<group>,<ip>,<port>` which is added to a vector of connections.
When the client issues the command `QUERYSERVERS,`, the server replies with a list of active connections. Querysevers must end with a comma!

## Final Product
We have successfully implemented all features of the server and client programs, server commands:
- `JOIN,<GROUP_ID>`
- `SERVERS,<serverlist>`
- `KEEPALIVE,<No. of Messages>`
- `FETCH_MSGS,<GROUP_ID>`
- `SEND_MSG,<TO_GROUP_ID>,<FROM_GROUP_ID>,<Message content>`
- `STATUSREQ,<FROM_GROUP_ID>`
- `STATUSRESP,FROM_GROUP,TO_GROUP,<group, msgs held>`

client commands:
- `FETCH,GROUP_ID`
- `SEND,GROUP_ID,<message contents>`
- `QUERYSERVERS`


We ran into some problems when trying to run our server on skel(130.208.243.61). It crashed, seemingly when someone tried to connect/send a command without the \x01 & \x04 tokens added. At least the output was always the same:
`Client connected on server: 6`
`no start token`
`no end token`
`[1]    3108172 segmentation fault (core dumped)` 
We were unable to find the cause for this segmentation fault and had a hard time recreating the fault when running multiple servers locally.

We were able to connect to a few servers and exchange some messages with a few of them. 



