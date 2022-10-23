// Server program for TSAM-409 Project 3
//
// Compile: g++ -std=c++11 server.cpp
//
// Authors:
// Reynir Gunnarsson (reynirg20@ru.is)
// Steinar Ísaksson (steinari20@ru.is)
// 
// Code modeled after example-2/server.cpp - Chat server for TSAM-409 by Jacky Mallet

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <vector>
#include <list>

#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <map>
#include <regex>

#include <unistd.h>

// fix SOCK_NONBLOCK for OSX
#ifndef SOCK_NONBLOCK
#include <fcntl.h>
#define SOCK_NONBLOCK O_NONBLOCK
#endif

#define BACKLOG  5          // Allowed length of queue of waiting connections
#define MAXSERVERS 16
#define GROUP_ID "P3_Group_20"

std::string theIPaddr;
std::string thePortInUse;

// Simple class for handling connections from clients.
//
// Client(int socket) - socket to send/receive traffic from client.
class Client
{
  public:
    int sock;              // socket of client connection
    std::string group_id;           // Limit length of name of client's user
    int port;
    std::string ip_num;
    int lastRcvdKeepAlive;

    Client(int socket){
        this->sock = socket;
        this->port = 0;
        this->ip_num = "";
        this->group_id = "";
        this->lastRcvdKeepAlive = 0;
    }

    Client(int socket, std::string ip_num, int port){
        this->sock = socket;
        this->port = port;
        this->ip_num = ip_num;
        this->group_id = "";
        this->lastRcvdKeepAlive = 0;
    }

    ~Client(){}            // Virtual destructor defined for base class
};

// Note: map is not necessarily the most efficient method to use here,
// especially for a server with large numbers of simulataneous connections,
// where performance is also expected to be an issue.
//
// Quite often a simple array can be used as a lookup table, 
// (indexed on socket no.) sacrificing memory for speed.

std::map<int, Client*> clients; // Lookup table for per Client information
std::map<std::string, std::vector<std::string> > messages;
int keepAliveMsgs = 0;


// Function to get time from UNIX epoch, 00:00:00, Jan 1st, 1970
int getTime()
{
    int retTime = (int)time(0);
    return retTime;
}


// Function to write timestamped log to file
void logger(char *buffer, std::string fromwho)
{
    std::ofstream log ("server_log.txt", std::ios_base::app);
    if (log.is_open())
    {
        std::time_t result = std::time(nullptr);
        log << fromwho <<std::asctime(std::localtime(&result)) << " " << buffer << std::endl;              
    }
}


// Open socket for specified port.
//
// Returns -1 if unable to create the socket for any reason.

std::string addTokens(std::string message)
{
    std::string theMessage = message;

    theMessage.erase(std::remove(theMessage.begin(), theMessage.end(), '\n'), theMessage.cend());

    std::string readyMsg = "";

    std::string startToken = "\x01";
    std::string endToken = "\x04";

    readyMsg = startToken + theMessage + endToken;

    return readyMsg;
}

std::string sanitizeMessage(char* buffer)
{
    std::string message = buffer;

    message.erase(remove(message.begin(), message.end(), '\n'), message.end());

    // ** CHECKING FOR TOKENS AT THE BEGINNING AND AT THE END ** //
    std::size_t index = message.find("\x01");
    if(index !=std::string::npos){
        message.erase(index,1); // erase function takes two parameter, the starting index in the string from where you want to erase characters and total no of characters you want to erase.
    }
    else
    {
        std::cout << "No start token" << std::endl;
        char rep[] = "No start token";
        logger(rep, "Server");
    }
    
    index = message.find("\x04");
    if (index != std::string::npos)
    {
        message.erase(index,1);
    }else
    {
        std::cout << "No end token" << std::endl;
        char rep[] = "No end token";
        logger(rep, "Server");
    }

    return message;
}

std::string eliminateSemiCommas(std::string buffer){
    std::regex pattern(";");
    std::string noCommas = std::regex_replace(buffer, pattern, ",");

    return noCommas;
}

int open_socket(int portno)
{
   struct sockaddr_in sk_addr;   // address settings for bind()
   int sock;                     // socket opened for this port
   int set = 1;                  // for setsockopt

   // Create socket for connection. Set to be non-blocking, so recv will
   // return immediately if there isn't anything waiting to be read.
#ifdef __APPLE__     
   if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
   {
      perror("Failed to open socket");
      return(-1);
   }
#else
   if((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
   {
     perror("Failed to open socket");
    return(-1);
   }
#endif

   // Turn on SO_REUSEADDR to allow socket to be quickly reused after 
   // program exit.

   if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
   {
      perror("Failed to set SO_REUSEADDR:");
   }
   set = 1;
#ifdef __APPLE__     
   if(setsockopt(sock, SOL_SOCKET, SOCK_NONBLOCK, &set, sizeof(set)) < 0)
   {
     perror("Failed to set SOCK_NOBBLOCK");
   }
#endif
   memset(&sk_addr, 0, sizeof(sk_addr));

   sk_addr.sin_family      = AF_INET;
   sk_addr.sin_addr.s_addr = INADDR_ANY;
   sk_addr.sin_port        = htons(portno);

   // Bind to socket to listen for connections from clients

   if(bind(sock, (struct sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
   {
      perror("Failed to bind to socket:");
      return(-1);
   }
   else
   {
      return(sock);
   }
}



// A monster function that generates a socket for new connections to other servers.

int getSocket(std::string ipAddr, std::string portNr){

    //std::cout << ipAddr << "-" << portNr;
    int newSocket;
    int set = 1;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    server = gethostbyname((theIPaddr.c_str()));

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
        (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
    serv_addr.sin_port = htons(stoi(portNr));

    #ifdef __APPLE__     
        if((newSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("Failed to open socket");
            // return(-1);
        }
    #else
        if((newSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
        {
            perror("Failed to open socket");
            return(-1);
        }
    #endif

        // Turn on SO_REUSEADDR to allow socket to be quickly reused after 
        // program exit.

        if(setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
        {
            perror("Failed to set SO_REUSEADDR:");
        }
        set = 1;
    #ifdef __APPLE__     
        if(setsockopt(newSocket, SOL_SOCKET, SOCK_NONBLOCK, &set, sizeof(set)) < 0)
        {
            perror("Failed to set SOCK_NOBBLOCK");
        }
    #endif

    std::cout << newSocket << std::endl;

    if(setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
   {
       printf("Failed to set SO_REUSEADDR for port %s\n", portNr.c_str());
       perror("setsockopt failed: ");
   }

    if(connect(newSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr) )< 0)
    {
       // EINPROGRESS means that the connection is still being setup. Typically this
       // only occurs with non-blocking sockets. (The serverSocket above is explicitly
       // not in non-blocking mode, so this check here is just an example of how to
       // handle this properly.)
       if(errno != EINPROGRESS)
       {
         printf("Failed to open socket to server: %s\n", ipAddr.c_str());
         perror("Connect failed: ");
       }
    }
    return newSocket;
}

// Close a client's connection, remove it from the client list, and
// tidy up select sockets afterwards.

void closeClient(int clientSocket, fd_set *openSockets, int *maxfds)
{

     printf("Client closed connection: %d\n", clientSocket);

     // If this client's socket is maxfds then the next lowest
     // one has to be determined. Socket fd's can be reused by the Kernel,
     // so there aren't any nice ways to do this.

     close(clientSocket);      

     if(*maxfds == clientSocket)
     {
        for(auto const& p : clients)
        {
            *maxfds = std::max(*maxfds, p.second->sock);
        }
     }

     // And remove from the list of open sockets.

     FD_CLR(clientSocket, openSockets);

}

void keepAlive(int clientSocket, std::string group_id)
{
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(20));
        std::string reply = "KEEPALIVE,";
        reply += std::to_string(messages[group_id].size());
        std::string tokenReply = addTokens(reply);
        send(clientSocket, tokenReply.c_str(), tokenReply.length(),0);
    }


}

// Process command from client on the server

void clientCommand(int clientSocket, fd_set *openSockets, int *maxfds, 
                  char *buffer) 
{
  std::vector<std::string> tokens;
  std::string token;
  std::string theBuffer = std::string(buffer);

//   std::string noSemiCommas = eliminateSemiCommas(std::string(buffer));

//   strcpy(buffer, noSemiCommas.c_str());
  size_t start;
  size_t end = 0;
  
  while ((start = theBuffer.find_first_not_of(',', end)) != std::string::npos)
  {
    end = theBuffer.find(',', start);
    tokens.push_back(theBuffer.substr(start, end - start));
  }

  // Split command from client into tokens for parsing
//   std::stringstream stream(buffer);

//   while(std::getline(stream, token, ','))
//     {
//         //std::cout << token << std::endl;
//         tokens.push_back(token);
//     }

  if((tokens[0].compare("FETCH") == 0) && (tokens.size() == 2))
  {
    if (messages.count(tokens[1]) != 0)
    {
        std::string reply = "Message recieved: ";
        reply += messages[tokens[1]][0];

        send(clientSocket, reply.c_str(), reply.length(),0);
    }
    
  }
  else if(tokens[0].compare("JOIN") == 0)
    {   

        std::thread(keepAlive,clientSocket,tokens[1]).detach();
        // Add this server as the first server in the reply

        std::string reply = "SERVERS,";
        reply += GROUP_ID;
        reply += ",";
        reply += theIPaddr;
        reply += ",";
        reply += thePortInUse;
        reply += ";";

        // Iterate through all the servers to add to the list
        for(auto const& names : clients)
        {
            if (std::to_string(names.second->port) != "0")
            {
                reply += names.second->group_id;
                reply += ",";
                reply += names.second->ip_num;
                reply += ",";
                reply += std::to_string(names.second->port);
                reply += ";";
            }
        }

        // for(auto const& pair: clients)
        // {
        //     reply += pair.second->group_id;
        //     reply += ",";
        //     reply += pair.second->ip_num;
        //     reply += ",";
        //     reply += pair.second->port;
        //     reply += ";";
            
        // }

        std::string tokenReply = addTokens(reply);

        send(clientSocket, tokenReply.c_str(), tokenReply.length(), 0);
    }
    else if(tokens[0].compare("SERVERS") == 0)
    {
        clients[clientSocket]->group_id = tokens[1];
        clients[clientSocket]->ip_num = tokens[2];
        clients[clientSocket]->port = stoi(tokens[3]);

        

        // if(tokens.size() > 4){
        //     for (int i = 4; i < token.size(); i = i + 3)
        //     {
        //         int newSocket = getSocket(tokens[i+1], tokens[i+2]);
                
        //         clients[newSocket] = new Client(newSocket);
                
        //         //Updating maxfds
        //         *maxfds = std::max(*maxfds, newSocket);
                
        //         //Adding out newSocket connection to openSockets.
        //         FD_SET(newSocket, openSockets);

        //         std::string reply = "JOIN,P3_Group_20";

        //         std::string tokenReply = addTokens(reply);

        //         send(newSocket, tokenReply.c_str(), tokenReply.length(), 0);

        //     }
            
        // }
    }
  else if((tokens[0].compare("SEND") == 0) && (tokens.size() == 3))
  {
      // Handle messages the client wants to send to other groups

    messages[tokens[1]].push_back(tokens[2]);

    // bool found = false;

    //  std::string reply = "";
    //  reply += "SEND_MSG,";
    //  reply += tokens[1];
    //  reply += ",";
    //  reply += GROUP_ID;
    //  reply += ",";
    //  reply += tokens[2];

     

     for(auto const& pair : clients)
      {
          if(pair.second->group_id.compare(tokens[1]) == 0)
          {
            std::string reply = "KEEPALIVE,";
            reply += std::to_string(messages[tokens[1]].size());
            std::string tokenReply = addTokens(reply);
            send(pair.second->sock, tokenReply.c_str(), tokenReply.length(),0);
            std::cout << "Message sent!" << std::endl;
            
            // found = true;
          }
      }
    //   if(!found){
    //     std::cout << "Group ID not connected to server" << std::endl;
    //     messages[tokens[1]].push_back(tokens[2]);
    //     keepAliveMsgs++;
    //   }

    
  }
  else if(tokens[0].compare("QUERYSERVERS") == 0)
  {
     //std::cout << "COMMAND " << tokens[0] << " not implemented." << std::endl;
     std::string reply;

     for(auto const& names : clients)
     {
        if (std::to_string(names.second->port) != "0")
        {
            reply += "Group ID: ";
            reply += names.second->group_id;
            reply += ", IP Address: ";
            reply += names.second->ip_num;
            reply += ", Port Number: ";
            reply += std::to_string(names.second->port);
            reply += "\n";
        }
     }
     // Reducing the msg length by 1 loses the excess "," - which
     // granted is totally cheating.
     std::string tokenReply = addTokens(reply);

     send(clientSocket, tokenReply.c_str(), tokenReply.length(), 0);

  }

  // A new command from client to connect to other servers.
  else if ((tokens[0].compare("CONNECT") == 0) && (tokens.size() == 3))
  {
    // Hardcoded join message that is sent to newly connected server immidiately after connecting.
    std::string reply = "JOIN,P3_GROUP_20";

    // Validating connection and making generating a secure socket to other server.
    int newSocket = getSocket(tokens[1], tokens[2]);
    
    // Adding the new connection to our client map
    clients[newSocket] = new Client(newSocket);
    //Updating maxfds
    *maxfds = std::max(*maxfds, newSocket) ;
    //Adding our newSocket connection to openSockets.
    FD_SET(newSocket, openSockets);

    std::string tokenReply = addTokens(reply);

    send(newSocket, tokenReply.c_str(), tokenReply.length(), 0);
  }
  else if ((tokens[0].compare("FETCH_MSGS") == 0) && (tokens.size() == 2))
  {
     if (messages.count(tokens[1]) != 0)
        {
            std::string reply = "SEND_MSG,";
            reply += tokens[1];
            reply += ",";
            reply += GROUP_ID;
            reply += ",";
            for (std::string s : messages[tokens[1]])
                {
                    reply += s;
                }
            
            std::string tokenReply = addTokens(reply);

            send(clientSocket, tokenReply.c_str(), tokenReply.length(),0);
            messages.erase(tokens[1]);
            
        }
  }
  else if ((tokens[0].compare("STATUSREQ") == 0) && (tokens.size() == 2))
  {
     std::string reply = "";
     reply += "STATUSRESP,";
     reply += GROUP_ID;
     reply += ",";
     reply += tokens[1];
     reply += ",";
     

     for (auto it = messages.begin(); it != messages.end(); ++it) 
      {
         reply += it->first + ",";
         reply += std::to_string(messages[it->first].size()) + ",";
      }
     reply.erase(std::remove(reply.begin(), reply.end(), '\n'), reply.cend());
     std::string tokenReply = addTokens(reply);
    
        send(clientSocket, tokenReply.c_str(), tokenReply.size(), 0);
  }
  else if ((tokens[0].compare("SEND_MSG") == 0))
  {

    messages[tokens[1]].push_back(tokens[3]);
    
    
  }
  else if ((tokens[0].compare("KEEPALIVE") == 0) && (tokens.size() > 1))
  {
    clients[clientSocket]->lastRcvdKeepAlive = getTime();
    if (stoi(tokens[1]) > 0)
    {
        std::string reply = "FETCH_MSGS,";
        reply += GROUP_ID;
        std::string tokenReply = addTokens(reply);
        send(clientSocket, tokenReply.c_str(), tokenReply.size(), 0);
    }
  }
  else
  {
      std::cout << "Unknown command from client:" << buffer << std::endl;
      char rep[] = "Unknown command from client:"; 
      logger(rep, "SERVER");
  }
     
}

int main(int argc, char* argv[])
{
    struct timeval time;

    bool finished;
    int listenSock;                 // Socket for connections to server
    int clientSock;                 // Socket of connecting client
    fd_set openSockets;             // Current open sockets 
    fd_set readSockets;             // Socket list for select()        
    fd_set exceptSockets;           // Exception socket list
    int maxfds;      
    int maxConn = 0;               // Passed to select() as max fd in set
    struct sockaddr_in client;
    socklen_t clientLen;
    char buffer[1025];              // buffer for reading from clients

    if(argc != 2)
    {
        printf("Usage: chat_server <ip port>\n");
        exit(0);
    }

    // Setup socket for server to listen to

    listenSock = open_socket(atoi(argv[1]));
    thePortInUse = argv[1];
    
    printf("Listening on port: %d\n", atoi(argv[1]));
    // for (int i=0;i<10;i++)
    // {
    //     std::string gr = "P3_GROUP_" + std::to_string(i);
    //     for (int j=0;j<3;j++)
    //     {
    //         messages[gr].push_back("jamm");
    //     }
    // };

    if(listen(listenSock, BACKLOG) < 0)
    {
        printf("Listen failed on port %s\n", argv[1]);
        exit(0);
    }
    else 
    // Add listen socket to socket set we are monitoring
    {
        if (maxConn <= MAXSERVERS)
        {
            FD_ZERO(&openSockets);
            FD_SET(listenSock, &openSockets);
            maxfds = listenSock;
        }
    }

    finished = false;

    while(!finished)
    {
        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openSockets;
        memset(buffer, 0, sizeof(buffer));

        // Look at sockets and see which ones have something to be read()
        int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

        if(n < 0)
        {
            perror("select failed - closing down\n");
            finished = true;
        }
        else
        {
            // First, accept  any new connections to the server on the listening socket
            if(FD_ISSET(listenSock, &readSockets))
            {
               clientSock = accept(listenSock, (struct sockaddr *)&client,
                                   &clientLen);
               
               // TRYING TO FETCH THE IP ADDRESS FROM HERE, DID NOT WORK UNFORTUNATELY
               //std::cout << client.sin_addr << std::endl;
               
               theIPaddr = inet_ntoa(((sockaddr_in)client).sin_addr);

               printf("accept***\n");
               // Add new client to the list of open sockets
               FD_SET(clientSock, &openSockets);
               // And update the maximum file descriptor
               maxfds = std::max(maxfds, clientSock) ;

               // create a new client to store information.
               clients[clientSock] = new Client(clientSock);

               // Decrement the number of sockets waiting to be dealt with
               n--;

               printf("Client connected on server: %d\n", clientSock);
            }
            std::list<Client *> disconnectedClients;

            // Check for KEEPALIVE times for all connected clients and disconnect if timed out

            for (auto const& pair : clients)
            {
                Client *client = pair.second;
                if (client->lastRcvdKeepAlive != 0 && getTime() - client->lastRcvdKeepAlive >= 60 )
                {
                    disconnectedClients.push_back(client);
                    closeClient(client->sock, &openSockets, &maxfds);
                }
            }
            // Now check for commands from clients
              
            while(n-- > 0)
            {
               for(auto const& pair : clients)
               {
                  Client *client = pair.second;

                  if(FD_ISSET(client->sock, &readSockets))
                  {
                      // recv() == 0 means client has closed connection
                      if(recv(client->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                      {
                          disconnectedClients.push_back(client);
                          closeClient(client->sock, &openSockets, &maxfds);

                      }
                      // We don't check for -1 (nothing received) because select()
                      // only triggers if there is something on the socket for us.
                      else
                      {
                          char commandFromClient[1025];
                          std::cout << buffer << std::endl;
                          std::string loggee = "Client(";
                          loggee += client->group_id;
                          loggee += ")";
                          logger(buffer, loggee);
                          std::string command = sanitizeMessage(buffer);
                          strcpy(commandFromClient, command.c_str());
                        //   std::cout << "Buffer: " << buffer << " with tokens: " << commandFromClient << std::endl;
                          clientCommand(client->sock, &openSockets, &maxfds, commandFromClient);
                      }
                  }
               }
                // Remove client from the clients list
                for(auto const& c : disconnectedClients)
                    clients.erase(c->sock);
            }
        }
    }
    
    return 0;
}
