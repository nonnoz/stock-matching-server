#include "server.h"



int main(){
    Database DB;
    


    //pugi::xml_document doc;
    //doc.load_file("test.xml");
    //parseXML(doc, DB);

    Server server;
    server.Run();



    return 0;
}

void somefunction(int user_input) {
    cout << user_input << endl; //print the user_input
    
    //do some processing here
}



