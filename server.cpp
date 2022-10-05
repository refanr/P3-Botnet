// Server program for TSAM-409 Project 3
//
// Compile: g++ -std=c++11 server.cpp
//
// Authors:
// Reynir Gunnarsson (reynirg20@ru.is)
// Steinar Ísaksson (steinari20@ru.is)
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#endif


// Predefined responses from the server:
char SUCCESS[] = "Command executed successfully\n";
char ERROR_MESSAGE[] = "Error executing command\n";
char UNKNOWN_MESSAGE[] = "Unknown command\n";

// Commands:    JOIN,<GROUP_ID>
//              SERVERS,<serverlist>
//              KEEPALIVE,<No. of Messages>
//              FETCH_MSGS,<GROUP_ID>
//              SEND_MSG,<TO_GROUP_ID>
//              STATUSREQ,<FROM_GROUP_ID>
//              STATUSRESP,FROM_GROUP,TO_GROUP,<group, msgs_held>
//              eg. STATUSRESP,P3_GROUP_2,I_1,P3_GROUP4,20,P3_GROUP71,2
//
// Client commands:
//              FETCH,GROUP_ID
//              SEND,GROUP_ID,<message contents>
//              QUERYSERVERS

