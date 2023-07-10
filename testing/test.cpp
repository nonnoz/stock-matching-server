#include <pthread.h>
#include <iostream>
#include <fstream>

#include "client.h"


//#define MULTI_THREAD true
//#define REQ_NUM 2500
#define SERVER_ADDR "vcm-32440.vm.duke.edu"


void *new_client(void *arg){
    int server_fd = get_connection(SERVER_ADDR);
    ifstream file((char*)arg);
    if(!file.is_open()){
        cout << "failed to open the request xml file" << endl;
        return NULL;
    }
    string requests, temp;
    while(getline(file, temp)){
        requests += temp;
        requests += '\n';
    }
    file.close();

    send(server_fd, requests.c_str(), requests.length(), 0);
    
    vector<char> buffer(65536);
    string response;

    while(1){
        buffer.clear();
        int len = recv(server_fd, buffer.data(), 65536, 0);
        if(len<=0){
            break;
        }
        response += string(buffer.begin(), buffer.begin()+len);
        cout << response << endl;
    }

}


int main(int argc, char **argv){
    if(argc != 2){
        cerr << "Invalid number of input!" << endl;
        exit(EXIT_FAILURE);
    }

    // while(1){
        int server_fd = get_connection(SERVER_ADDR);
        ifstream file(argv[1]);
        if(!file.is_open()){
            cout << "failed to open the request xml file" << endl;
            return EXIT_FAILURE;
        }
        string requests, temp;
        while(getline(file, temp)){
            //cout << temp << endl;
            requests += temp;
            requests += '\n';
        }
        file.close();

        send(server_fd, requests.c_str(), requests.length(), 0);
        
        vector<char> buffer(65536);
        string response;
        buffer.clear();
        int len = recv(server_fd, buffer.data(), 65536, 0);
        // if(len<=0){
        //     break;
        // }
        response += string(buffer.begin(), buffer.begin()+len);
        cout << response << endl;
    // }

    return 0;

}



// int server_fd = get_connection(SERVER_ADDR);
//     ifstream file(argv[1]);
//     if(!file.is_open()){
//         cout << "failed to open the request xml file" << endl;
//         return EXIT_FAILURE;
//     }
//     string requests, temp;
//     while(getline(file, temp)){
//         //cout << temp << endl;
//         requests += temp;
//         requests += '\n';
//     }
//     file.close();
    
//     int req_num = stoi(argv[2]);
    
//     if(!req_num){   //multi_thread
//         send(server_fd, requests.c_str(), requests.length(), 0);
            
//         vector<char> buffer(65536);
//         string response;
//         buffer.clear();
//         int len = recv(server_fd, buffer.data(), 65536, 0);
//         // if(len<=0){
//         //     break;
//         // }
//         response += string(buffer.begin(), buffer.begin()+len);
//         cout << response << endl;
//     }
//     else{
//         for(int i = 0; i < req_num; i++){
//             send(server_fd, requests.c_str(), requests.length(), 0);
            
//             vector<char> buffer(65536);
//             string response;
//             buffer.clear();
//             int len = recv(server_fd, buffer.data(), 65536, 0);
//             // if(len<=0){
//             //     break;
//             // }
//             response += string(buffer.begin(), buffer.begin()+len);
//             cout << response << endl;
//         }
//     }