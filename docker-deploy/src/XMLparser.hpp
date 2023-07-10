#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include<iostream>
#include <vector>
#include <unordered_map>

#include "pugixml.hpp"
#include "pugiconfig.hpp"

//#include "funcs.hpp"

#include "database.h"
#include <mutex>


using namespace std;

pugi::xml_document parseXML(pugi::xml_document &doc, Database &DB);
void createAccount(pugi::xml_node &create, Database &DB, pugi::xml_node &res_root);
void createSymbol(pugi::xml_node &create, Database &DB, pugi::xml_node &res_root);
void trans_order(pugi::xml_node &trans, Database &DB, pugi::xml_node &res_root, string account_id);
void trans_query(pugi::xml_node &trans, Database &DB, pugi::xml_node &res_root, string account_id);
void trans_cancel(pugi::xml_node &trans, Database &DB, pugi::xml_node &res_root, string account_id);