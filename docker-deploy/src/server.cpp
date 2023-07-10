#include "server.h"
#include <thread>
#include <pthread.h>
// std::mutex mtx;
// pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void Server::response(int client_fd){
    vector<char> buffer(65536);
    string recerivedData;

    // while(1){
    //     buffer.clear();
    //     int len = recv(client_fd, buffer.data(), 65536, 0);
    //     if(len<=0){
    //         break;
    //     }

    //     recerivedData += string(buffer.data());
    // }

    recv(client_fd, buffer.data(), 65536, 0);
    recerivedData = buffer.data();


    pugi::xml_document doc;
    doc.load_string(recerivedData.data());

    stringstream buf;
    // mtx.lock();
    pugi::xml_document results = parseXML(doc, DB);
    // mtx.unlock();
    results.save(buf);
    string response = buf.str();

    send(client_fd, response.data(), response.size(), 0);

}

void Server::DBInit(){
    // string postgresql = "dbname=exchange user=postgres password=passw0rd";
    string postgresql = "dbname=exchange user=postgres password=passw0rd host=db port=5432";
    DB.connect(postgresql);
    DB.dropTables();
    DB.createTables();
}

Server::Server(){
    DBInit();
    struct addrinfo host_info;
    int status;
    const char *hostname = NULL;
    struct addrinfo *host_info_list;

    
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;


    
    //assign the address of my local host to the socket structures. 
    status = getaddrinfo(hostname, SERVER_PORT, &host_info, &host_info_list);
    if (status != 0) {
        cerr << "Error: cannot get address info for host" << endl;
        cerr << "  (" << hostname << "," << SERVER_PORT << ")" << endl;
        exit(EXIT_FAILURE);
    } 

    //get the socket File Descriptor
    listen_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
    if (listen_fd == -1) {
        cerr << "Error: cannot create socket" << endl;
        cerr << "  (" << hostname << "," << SERVER_PORT << ")" << endl;
        exit(EXIT_FAILURE);
    }
    
    //if a little bit of a socket that was connected is still hanging around in the kernel
    //and it’s hogging the port. This allows it to reuse the port
    int yes = 1;
    status = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (status == -1) {
        cerr << "Error: cannot setsockopt" << endl;
        exit(EXIT_FAILURE);
    }
    //bind to the IP of the host it’s running on
    status = bind(listen_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        cerr << "Error: cannot bind socket" << endl;
        cerr << "  (" << hostname << "," << SERVER_PORT << ")" << endl;
        exit(EXIT_FAILURE);
    } 



    status = listen(listen_fd, num_listen);
    if (status == -1) {
        cerr << "Error: cannot listen on socket" << endl; 
        cerr << "  (" << hostname << "," << SERVER_PORT << ")" << endl;
        exit(EXIT_FAILURE);
    }


    freeaddrinfo(host_info_list); // free the linked list

}



int Server::accept_connection(){
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int client_connection_fd, status;
    
    client_connection_fd = accept(listen_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    
    if (client_connection_fd == -1) {
        cout << "Error: cannot accept connection on socket" << endl;
        return accept_connection();
    }

    return client_connection_fd;

}

// void Server::Run(Database DB){
//     #if SCALABILITY
//         threadpool executor{50};
//     #endif
    
//     while(1){
//         cout << "while1" << endl;
//         int client_fd = accept_connection();

//         cout << "client_fd" << endl;

//         #if SCALABILITY
//             executor.commit(response, client_fd, DB);
//         #else
//             thread thread(response, client_fd, DB);
//             thread.join();
//         #endif
//     }
//     close(listen_fd);
// }

void Server::Run(){

    int i = 0;
    while(1){
        //pthread_mutex_lock(&lock);
        i++;
        int client_fd = accept_connection();
        thread t;
        
        try{
            t = std::thread(&Server::response, this, client_fd);
            cout << "Client " << i << " replied." << endl;
        }

        catch(std::runtime_error& e){
            t.detach();
            continue;
        }
        t.detach();
        //pthread_mutex_unlock(&lock);

    }
    close(listen_fd);
}
