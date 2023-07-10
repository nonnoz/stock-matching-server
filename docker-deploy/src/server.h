#include "XMLparser.hpp"

#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <algorithm>
#include <sstream>
// #include <mutex>


#define SCALABILITY 0

#define THREAD_POOL 1

#define SERVER_PORT "12345"

class Server{
public:    
    int listen_fd;
    //const char* its_port;
    //string its_port;
    int num_listen;
    int port_int;
    Database DB;
void DBInit();
    Server();
    //int socket_create();
    int accept_connection();
    void Run();
    void response(int client_fd);

};