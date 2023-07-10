#include "../server.h"


#define SERVER_PORT "12345"

int get_connection(const char * hostname){
        int status;
        int socket_fd;
        struct addrinfo host_info;
        struct addrinfo *host_info_list;
 

        memset(&host_info, 0, sizeof(host_info));
        host_info.ai_family   = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;

        // load up address structs 
        status = getaddrinfo(hostname, SERVER_PORT, &host_info, &host_info_list);
        if (status != 0) {
            cerr << "Error: cannot get address info for host" << endl;
            cerr << "  (" << hostname << "," << SERVER_PORT << ")" << endl;
            exit(EXIT_FAILURE);
        } //if

        //// make a socket:
        socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
        if (socket_fd == -1) {
            cerr << "Error: cannot create socket" << endl;
            cerr << "  (" << hostname << "," << SERVER_PORT << ")" << endl;
            exit(EXIT_FAILURE);
        } //if


        status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
        if (status == -1) {
            cerr << "Error: cannot connect to socket" << endl;
            cerr << "  (" << hostname << "," << SERVER_PORT << ")" << endl;
            exit(EXIT_FAILURE);
        } //if

        

        //freeaddrinfo(host_info_list);

        return socket_fd;
    }
