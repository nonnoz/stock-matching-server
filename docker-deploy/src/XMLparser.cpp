#include "XMLparser.hpp"
std::mutex xml_lock;


pugi::xml_document parseXML(pugi::xml_document &doc, Database &DB){

    std::lock_guard<std::mutex> lock(xml_lock);
    pugi::xml_document results;
    for(pugi::xml_node root_node = doc.first_child(); root_node; root_node = root_node.next_sibling()){
        
        //for every create request
        if(!strcmp(root_node.name(), "create")){
            pugi::xml_node res_root = results.append_child("results");
        
            for(pugi::xml_node create = root_node.first_child(); create; create = create.next_sibling()){
                if(!strcmp(create.name(),"account")){           //create account
                    createAccount(create, DB, res_root);
                }
                else if(!strcmp(create.name(),"symbol")){       //create symbol     
                    createSymbol(create, DB, res_root);
                }
            } 
        }
        else if(!strcmp(root_node.name(), "transactions")){
            pugi::xml_node res_root = results.append_child("results");

            string account_id = root_node.attribute("id").value();

            for(pugi::xml_node trans = root_node.first_child(); trans; trans = trans.next_sibling()){
                if(!strcmp(trans.name(),"order")){             //order
                    trans_order(trans, DB, res_root, account_id);
                }
                else if(!strcmp(trans.name(),"query")){
                    trans_query(trans, DB, res_root, account_id);
                }
                else if(!strcmp(trans.name(),"cancel")){
                    trans_cancel(trans, DB, res_root, account_id);

                }
            }
        }
    }
    //results.save_file("results.xml");
    return results;

}


void createAccount(pugi::xml_node &create, Database &DB, pugi::xml_node &res_root){
    string account_id = create.attribute("id").value();
    double balance = stod(create.attribute("balance").value());

    //if account exists ?   
    //bool accountExist(string acc_id);
    if(DB.accountExist(account_id)){
        pugi::xml_node act_err = res_root.append_child("error");
        act_err.append_attribute("id").set_value(account_id.c_str());
        act_err.text().set("Account already exists");
    }
    else{
        //void createAccount(string acc_id, int balance);
        if(DB.createAccount(account_id, balance)){ //created successfully
            res_root.append_child("created").append_attribute("id").set_value(account_id.c_str());
        }
        else{
            pugi::xml_node act_err = res_root.append_child("error");
            act_err.append_attribute("id").set_value(account_id.c_str());
            act_err.text().set("Creating account failed");
        }
    }
}

void createSymbol(pugi::xml_node &create, Database &DB, pugi::xml_node &res_root){
    string sym = create.attribute("sym").value();

    //to each account
    for(pugi::xml_node sym_child = create.first_child(); sym_child; sym_child = sym_child.next_sibling()){
        if(!strcmp(sym_child.name(), "account")){
            string account_id = sym_child.attribute("id").value();
            double amount = stod(sym_child.text().data().value());

            //find the account
            if (DB.accountExist(account_id)){
                if(DB.changePosition(account_id, sym, amount)){
                    pugi::xml_node sym_created = res_root.append_child("created");
                    sym_created.append_attribute("sym").set_value(sym.c_str());
                    sym_created.append_attribute("id").set_value(account_id.c_str());
                }
                else{ //changing failed
                    pugi::xml_node sym_err = res_root.append_child("error");
                    sym_err.append_attribute("sym").set_value(sym.c_str());
                    sym_err.append_attribute("id").set_value(account_id.c_str());
                    sym_err.text().set("Position changing failed");
                }
                
            }
            else{
                pugi::xml_node sym_err = res_root.append_child("error");
                sym_err.append_attribute("sym").set_value(sym.c_str());
                sym_err.append_attribute("id").set_value(account_id.c_str());
                sym_err.text().set("Account not exists");
            }
        }     
    }
}

void trans_order(pugi::xml_node &trans, Database &DB, pugi::xml_node &res_root, string account_id){
    pugi::xml_attribute attribute = trans.first_attribute();
    string sym = attribute.value();
    attribute = attribute.next_attribute();
    //cout << "amount: " << attribute.value() << "    limit: " << attribute.next_attribute().value() << endl;
    double amount = stod(attribute.value());
    attribute = attribute.next_attribute();
    double limit = stod(attribute.value());


    if(!DB.accountExist(account_id)){
        pugi::xml_node order_err = res_root.append_child("error");
        order_err.append_attribute("sym").set_value(sym.c_str());
        order_err.append_attribute("amount").set_value(amount);
        order_err.append_attribute("limit").set_value(limit);
        order_err.text().set("Invalid account id");
        return;
    }
    if(amount > 0){
        if(!DB.buyValid(account_id, amount, limit)){
            pugi::xml_node order_err = res_root.append_child("error");
            order_err.append_attribute("sym").set_value(sym.c_str());
            order_err.append_attribute("amount").set_value(amount);
            order_err.append_attribute("limit").set_value(limit);
            order_err.text().set("Insufficient funds are available");
            return;
        }
    }
    else{
        if(!DB.sellValid(account_id, sym, amount)){
            pugi::xml_node order_err = res_root.append_child("error");
            order_err.append_attribute("sym").set_value(sym.c_str());
            order_err.append_attribute("amount").set_value(amount);
            order_err.append_attribute("limit").set_value(limit);
            order_err.text().set("Insufficient shares are available");
            return;
        }
    }

    long trans_id = DB.createOrder(account_id, sym, amount, limit);
    pugi::xml_node order_open = res_root.append_child("opened");
    order_open.append_attribute("sym").set_value(sym.c_str());
    order_open.append_attribute("amount").set_value(amount);
    order_open.append_attribute("limit").set_value(limit);
    order_open.append_attribute("id").set_value(trans_id);
    DB.handleOrder(trans_id);
}



void trans_query(pugi::xml_node &trans, Database &DB, pugi::xml_node &res_root, string account_id){

    long trans_id = stol(trans.attribute("id").value());
    pugi::xml_node status_node = res_root.append_child("status");
    status_node.append_attribute("id").set_value(trans_id);
    
    if(!DB.accountExist(account_id)){

        pugi::xml_node err = status_node.append_child("error");
        err.append_attribute("id").set_value(trans_id);
        err.text().set("Invalid account id");
        return;
    }

    if(!DB.transExist(trans_id)){
        pugi::xml_node err = status_node.append_child("error");
        err.append_attribute("id").set_value(trans_id);
        err.text().set("Invalid transaction id");
        return;
    }

    if(!DB.checkAccTrans(account_id, trans_id)){
        pugi::xml_node err = status_node.append_child("error");
        err.append_attribute("id").set_value(trans_id);
        err.text().set("Invalid transaction id for this account");
        return;
    }

    //status
    vector<Status> statuses = DB.query(trans_id);
    for(int i = 0; i < statuses.size(); i++){
        if(statuses[i].status == OPEN){
            status_node.append_child("open").append_attribute("shares").set_value(statuses[i].amount);
        }
        else if(statuses[i].status == CANCELED){
            pugi::xml_node canceled_status = status_node.append_child("canceled");
            canceled_status.append_attribute("shares").set_value(statuses[i].amount);
            canceled_status.append_attribute("time").set_value(statuses[i].time);
        }
        else if(statuses[i].status == EXECUTED){
            pugi::xml_node executed_status = status_node.append_child("executed");
            executed_status.append_attribute("shares").set_value(statuses[i].amount);
            executed_status.append_attribute("price").set_value(statuses[i].price);
            executed_status.append_attribute("time").set_value(statuses[i].time);
        }
    }
}


void trans_cancel(pugi::xml_node &trans, Database &DB, pugi::xml_node &res_root, string account_id){

    long trans_id = stol(trans.attribute("id").value());
    pugi::xml_node cancel_node = res_root.append_child("canceled");
    cancel_node.append_attribute("id").set_value(trans_id);

    if(!DB.accountExist(account_id)){

        pugi::xml_node err = cancel_node.append_child("error");
        err.append_attribute("id").set_value(trans_id);
        err.text().set("Invalid account id");
        return;
    }

    if(!DB.transExist(trans_id)){
        pugi::xml_node err = cancel_node.append_child("error");
        err.append_attribute("id").set_value(trans_id);
        err.text().set("Invalid transaction id");
        return;
    }

    if(!DB.checkAccTrans(account_id, trans_id)){
        pugi::xml_node err = cancel_node.append_child("error");
        err.append_attribute("id").set_value(trans_id);
        err.text().set("Invalid transaction id for this account");
        return;
    }

    //ask for cancellation
    vector<Status> statuses = DB.cancel(trans_id);
    for(int i = 0; i < statuses.size(); i++){
        if(statuses[i].status == CANCELED){
            pugi::xml_node canceled_status = cancel_node.append_child("canceled");
            canceled_status.append_attribute("shares").set_value(statuses[i].amount);
            canceled_status.append_attribute("time").set_value(statuses[i].time);
        }
        else if(statuses[i].status == EXECUTED){
            pugi::xml_node executed_status = cancel_node.append_child("executed");
            executed_status.append_attribute("shares").set_value(statuses[i].amount);
            executed_status.append_attribute("price").set_value(statuses[i].price);
            executed_status.append_attribute("time").set_value(statuses[i].time);
        }
    }
}