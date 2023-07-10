#include "database.h"
// std::mutex mtx;

void Database::connect(string db_param)
{
    try
    {
        // Establish a connection to the database
        // Parameters: database name, user name, user password
        C = new connection(db_param);
        if (C->is_open())
        {
            cout << "Opened database successfully: " << C->dbname() << endl;
            stringstream querySerial;
            querySerial << "BEGIN ISOLATION LEVEL SERIALIZABLE;";
            execQuery(querySerial.str());
        }
        else
        {
            cout << "Can't open database" << endl;
            return;
        }
    }
    catch (const std::exception &e)
    {
        cerr << e.what() << std::endl;
        return;
    }
}

void Database::execQuery(string query)
{
    work W(*C);
    W.exec(query);
    W.commit();
}

// result Database::select(string query)
// {
//     work W(*C);
//     result R = W.exec(query);
//     W.commit();
//     return R;
// }

bool Database::accountExist(string acc_id)
{

    /* Create a non-transactional object. */
    nontransaction N(*C);

    stringstream query;
    query << "SELECT * FROM ACCOUNT WHERE ACCOUNT_ID=" << N.quote(acc_id) << ";";

    /* Execute SQL query */
    result R(N.exec(query));

    if (R.size() == 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool Database::createAccount(string acc_id, double balance)
{
    try
    {
        work W(*C);
        stringstream query;
        query
            << "INSERT INTO ACCOUNT (ACCOUNT_ID, BALANCE) VALUES ("
            << W.quote(acc_id) << ", " << to_string(balance) << ");";
        W.exec(query);
        W.commit();
    }
    catch (const std::exception &e)
    {
        cerr << e.what() << std::endl;
        return false;
    }
    return true;
}

bool Database::checkAccTrans(string acc_id, long trans_id)
{
    nontransaction N(*C);

    stringstream query;
    query << "SELECT * FROM ORDERS WHERE ACCOUNT_ID=" << N.quote(acc_id)
          << " AND TRANS_ID=" << to_string(trans_id) << ";";

    result R(N.exec(query));

    if (R.size() == 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

void Database::updateBalance(string acc_id, double balance)
{

    work W(*C);
    stringstream query;
    query
        << "UPDATE ACCOUNT SET BALANCE=" << to_string(balance)
        << " WHERE ACCOUNT_ID=" << W.quote(acc_id) << ";";
    W.exec(query);
    W.commit();
}

bool Database::positionExist(string acc_id, string sym)
{
    nontransaction N(*C);
    stringstream query;
    query << "SELECT * FROM POSITION WHERE ACCOUNT_ID=" << N.quote(acc_id)
          << " AND SYMBOL=" << N.quote(sym) << ";";

    result R(N.exec(query));
    if (R.size() == 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

double Database::getPositionAmount(string acc_id, string sym)
{
    double ans;
    nontransaction N(*C);
    stringstream query;
    query << "SELECT AMOUNT FROM POSITION WHERE ACCOUNT_ID=" << N.quote(acc_id)
          << " AND SYMBOL=" << N.quote(sym) << " FOR UPDATE;";

    result R(N.exec(query));

    for (result::const_iterator c = R.begin(); c != R.end(); ++c)
    {
        ans = c[0].as<double>();
    }

    return ans;
}

bool Database::createPosition(string acc_id, string sym, double amount)
{
    try
    {
        work W(*C);
        stringstream query;
        query
            << "INSERT INTO POSITION (ACCOUNT_ID, SYMBOL, AMOUNT) VALUES ("
            << W.quote(acc_id) << ", " << W.quote(sym) << ", " << to_string(amount) << ");";
        W.exec(query);
        W.commit();
    }
    catch (const std::exception &e)
    {
        cerr << e.what() << std::endl;
        return false;
    }
    return true;
}

void Database::updatePosition(string acc_id, string sym, double amount)
{

    work W(*C);
    stringstream query;
    query
        << "UPDATE POSITION SET AMOUNT=" << to_string(amount)
        << " WHERE ACCOUNT_ID=" << W.quote(acc_id)
        << " AND SYMBOL=" << W.quote(sym) << ";";
    W.exec(query);
    W.commit();
}

bool Database::changePosition(string acc_id, string sym, double amount)
{
    try
    {
        if (positionExist(acc_id, sym))
        {
            // update amount
            double new_amount = getPositionAmount(acc_id, sym) + amount;
            updatePosition(acc_id, sym, new_amount);
        }
        else
        {
            // create new position
            createPosition(acc_id, sym, amount);
        }
    }
    catch (const std::exception &e)
    {
        cerr << e.what() << std::endl;
        return false;
    }
    return true;
}

double Database::getAccBalance(string acc_id)
{
    nontransaction N(*C);
    stringstream query;
    query << "SELECT BALANCE FROM ACCOUNT WHERE ACCOUNT_ID=" << N.quote(acc_id) << " FOR UPDATE;";
    result R(N.exec(query));
    double balance;
    for (result::const_iterator c = R.begin(); c != R.end(); ++c)
    {
        balance = c[0].as<double>();
    }
    return balance;
}

bool Database::buyValid(string acc_id, double amount, double price)
{
    double balance = getAccBalance(acc_id);

    if (balance < amount * price)
    {
        return false;
    }
    else
    {
        double new_balance = balance - amount * price;
        updateBalance(acc_id, new_balance);
        return true;
    }
}

bool Database::sellValid(string acc_id, string sym, double amount)
{
    if (!positionExist(acc_id, sym))
    {
        return false;
    }

    double my_amount = getPositionAmount(acc_id, sym);

    if (my_amount < -amount)
    {
        return false;
    }
    else
    {
        double new_amount = my_amount + amount;
        updatePosition(acc_id, sym, new_amount);
        return true;
    }
}

long Database::createOrder(string acc_id, string sym, double amount, double price)
{
    long trans_id = -1;
    try
    {
        work W(*C);
        stringstream query;
        time_t t = time(NULL);
        query
            << "INSERT INTO ORDERS (ACCOUNT_ID, SYMBOL, AMOUNT, PRICE, TIME, STATUS) VALUES ("
            << W.quote(acc_id) << ", "
            << W.quote(sym) << ", "
            << to_string(amount) << ", "
            << to_string(price) << ", "
            << to_string(t) << ", "
            << W.quote("OPEN") << ");";
        W.exec(query);
        W.commit();

        nontransaction N(*C);
        stringstream get_id;
        get_id << "SELECT TRANS_ID FROM ORDERS WHERE TIME=" << to_string(t) << ";";

        result R(N.exec(get_id));
        for (result::const_iterator c = R.begin(); c != R.end(); ++c)
        {
            trans_id = c[0].as<long>();
        }
    }
    catch (const std::exception &e)
    {
        cerr << e.what() << std::endl;
        return -1;
    }
    return trans_id;
}

double Database::getOrderAmount(long trans_id)
{
    double ans;

    nontransaction N(*C);
    stringstream query;
    query << "SELECT AMOUNT FROM ORDERS WHERE TRANS_ID=" << to_string(trans_id) << ";";
    result R(N.exec(query));
    for (result::const_iterator c = R.begin(); c != R.end(); ++c)
    {
        ans = c[0].as<double>();
    }

    return ans;
}

long Database::matchForBuyer(long trans_id)
{
    // if no match, return -1
    long match_id = -1;
    string sym;
    double buyer_price;

    // get buyer symbol and price
    work W0(*C);
    stringstream query_get;
    query_get << "SELECT SYMBOL, PRICE FROM ORDERS WHERE TRANS_ID=" << to_string(trans_id) << ";";
    result R_get = W0.exec(query_get);
    W0.commit();

    for (result::const_iterator c = R_get.begin(); c != R_get.end(); ++c)
    {
        sym = c[0].as<string>();
        buyer_price = c[1].as<double>();
    }

    // select sell orders (AMOUNT < 0), PRICE <= buyer_price, match SYMBOL, the order is open, then choose the cheapest one
    work W1(*C);
    stringstream query_sel;
    query_sel << "SELECT TRANS_ID FROM ORDERS WHERE SYMBOL=" << W1.quote(sym)
              << " AND AMOUNT<0 AND PRICE<=" << to_string(buyer_price) << " AND STATUS=" << W1.quote("OPEN") << " ORDER BY PRICE ASC, TIME ASC;";
    result R_sel = W1.exec(query_sel);
    W1.commit();

    // do have match
    if (R_sel.size() > 0)
    {
        // get the first (cheapest) one
        result::const_iterator c = R_sel.begin();
        match_id = c[0].as<double>();
    }

    return match_id;
}

long Database::matchForSeller(long trans_id)
{
    // if no match, return -1
    long match_id = -1;
    string sym;
    double seller_price;

    // get seller price
    work N0(*C);
    stringstream query_get;
    query_get << "SELECT SYMBOL, PRICE FROM ORDERS WHERE TRANS_ID=" << to_string(trans_id) << ";";
    result R_get = N0.exec(query_get);
    N0.commit();

    for (result::const_iterator c = R_get.begin(); c != R_get.end(); ++c)
    {
        sym = c[0].as<string>();
        seller_price = c[1].as<double>();
    }

    // select buy orders (AMOUNT > 0), PRICE >= seller_price, match SYMBOL, the order is open, then choose the highest one
    work N1(*C);
    stringstream query_sel;
    query_sel << "SELECT TRANS_ID FROM ORDERS WHERE SYMBOL=" << N1.quote(sym)
              << " AND AMOUNT>0 AND PRICE>=" << to_string(seller_price) << " AND STATUS=" << N1.quote("OPEN") << " ORDER BY PRICE DESC, TIME ASC;";
    result R_sel = N1.exec(query_sel);
    N1.commit();

    // do have match
    if (R_sel.size() > 0)
    {
        // get the first (highest) one
        result::const_iterator c = R_sel.begin();
        match_id = c[0].as<double>();
    }

    return match_id;
}

bool Database::transExist(long trans_id)
{
    nontransaction N(*C);

    stringstream query;
    query << "SELECT * FROM ORDERS WHERE TRANS_ID=" << to_string(trans_id) << ";";

    result R(N.exec(query));

    if (R.size() == 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

Order Database::getOrderInfo(long trans_id)
{
    Order ans;

    nontransaction N(*C);
    stringstream query;
    query << "SELECT ACCOUNT_ID, SYMBOL, AMOUNT, PRICE, TIME FROM ORDERS WHERE TRANS_ID=" << to_string(trans_id) << " FOR UPDATE;";
    result R(N.exec(query));
    for (result::const_iterator c = R.begin(); c != R.end(); ++c)
    {
        ans = {c[0].as<string>(), c[1].as<string>(), c[2].as<double>(), c[3].as<double>(), c[4].as<long>()};
    }

    return ans;
}

void Database::executeOrder(long buy_id, long sell_id, bool isBuy)
{
    time_t t = time(NULL);

    // get buy and sell amount and price
    Order buy_info = getOrderInfo(buy_id);
    Order sell_info = getOrderInfo(sell_id);

    double buy_amount = buy_info.amount;
    double sell_amount = sell_info.amount;
    double sell_pos = -sell_amount;

    double buy_price = buy_info.price;
    double sell_price = sell_info.price;

    // long buy_time = buy_info.time;
    // long sell_time = sell_info.time;

    double execution_price;

    if (isBuy)
    {
        // sell order opens first
        // use sell price
        execution_price = sell_price;
    }
    else
    {
        // buy order opens first
        // use buy price
        execution_price = buy_price;
    }

    // add 2 to execute

    work W(*C);
    stringstream insert_buy;
    stringstream insert_sell;
    stringstream mod_buy;
    stringstream mod_sell;

    mod_buy
        << "UPDATE ORDERS SET STATUS=" << W.quote("EXECUTED")
        << " WHERE TRANS_ID=" << to_string(buy_id) << ";";

    mod_sell
        << "UPDATE ORDERS SET STATUS=" << W.quote("EXECUTED")
        << " WHERE TRANS_ID=" << to_string(sell_id) << ";";

    W.commit();

    if (buy_amount == sell_pos)
    {
        // same, add 2 to exe, modify 2 status in order

        work W(*C);

        insert_buy
            << "INSERT INTO EXECUTE (TRANS_ID, AMOUNT, PRICE, TIME) VALUES ("
            << W.quote(buy_id) << ", "
            << to_string(buy_amount) << ", "
            << to_string(execution_price) << ", "
            << to_string((long)t) << ");";

        insert_sell
            << "INSERT INTO EXECUTE (TRANS_ID, AMOUNT, PRICE, TIME) VALUES ("
            << W.quote(sell_id) << ", "
            << to_string(buy_amount) << ", "
            << to_string(execution_price) << ", "
            << to_string((long)t) << ");";

        W.commit();

        execQuery(insert_buy.str());
        execQuery(insert_sell.str());

        // if isBuy
        // sell order opens first
        // use sell price
        // add seller balance and buyer get refund

        // if not isBuy
        // buy order opens first
        // use buy price
        // add seller balance

        // modify status in order
        // all change to executed

        execQuery(mod_buy.str());

        execQuery(mod_sell.str());

        // add position to buyer
        changePosition(buy_info.acc_id, buy_info.sym, buy_amount);
        // add money to seller
        stringstream seller_add_balance;
        double new_balance = getAccBalance(sell_info.acc_id) + execution_price * buy_amount;
        updateBalance(sell_info.acc_id, new_balance);

        // if isBuy
        // sell order opens first
        // buyer get refund
        if (isBuy)
        {
            double old_balance = getAccBalance(buy_info.acc_id);
            double refund = (buy_price - execution_price) * buy_amount;
            updateBalance(buy_info.acc_id, old_balance + refund);
        }
        return;
    }
    else if (buy_amount < sell_pos)
    {
        // seller has more amount
        // use buyer amount
        // insert 2

        work W(*C);

        insert_buy
            << "INSERT INTO EXECUTE (TRANS_ID, AMOUNT, PRICE, TIME) VALUES ("
            << W.quote(buy_id) << ", "
            << to_string(buy_amount) << ", "
            << to_string(execution_price) << ", "
            << to_string((long)t) << ");";

        insert_sell
            << "INSERT INTO EXECUTE (TRANS_ID, AMOUNT, PRICE, TIME) VALUES ("
            << W.quote(sell_id) << ", "
            << to_string(buy_amount) << ", "
            << to_string(execution_price) << ", "
            << to_string((long)t) << ");";
        W.commit();

        execQuery(insert_buy.str());

        execQuery(insert_sell.str());

        // mod buy_id in order to EXECUTED
        execQuery(mod_buy.str());

        // update seller order amount
        double new_amount = sell_amount + buy_amount;
        stringstream update_sell_amount;
        update_sell_amount
            << "UPDATE ORDERS SET AMOUNT=" << to_string(new_amount)
            << " WHERE TRANS_ID=" << to_string(sell_id) << ";";
        execQuery(update_sell_amount.str());

        // add position to buyer
        changePosition(buy_info.acc_id, buy_info.sym, buy_amount);
        // add money to seller
        stringstream seller_add_balance;
        double new_balance = getAccBalance(sell_info.acc_id) + execution_price * buy_amount;
        updateBalance(sell_info.acc_id, new_balance);

        // if isBuy, refund
        if (isBuy)
        {
            double old_balance = getAccBalance(buy_info.acc_id);
            double refund = (buy_price - execution_price) * buy_amount;
            updateBalance(buy_info.acc_id, old_balance + refund);
        }

        // sell has more, match for sell
        long match_id = matchForSeller(sell_id);

        // if !isBuy and match, executeOrder again
        if (!isBuy && match_id != -1)
        {
            executeOrder(match_id, sell_id, false);
        }
        else
        {
            return;
        }
    }
    else
    {
        // buyer wants more
        // use sell amount

        // insert 2

        work W(*C);

        insert_buy
            << "INSERT INTO EXECUTE (TRANS_ID, AMOUNT, PRICE, TIME) VALUES ("
            << W.quote(buy_id) << ", "
            << to_string(sell_pos) << ", "
            << to_string(execution_price) << ", "
            << to_string((long)t) << ");";

        insert_sell
            << "INSERT INTO EXECUTE (TRANS_ID, AMOUNT, PRICE, TIME) VALUES ("
            << W.quote(sell_id) << ", "
            << to_string(sell_pos) << ", "
            << to_string(execution_price) << ", "
            << to_string((long)t) << ");";

        W.commit();

        execQuery(insert_buy.str());

        execQuery(insert_sell.str());

        // mod sell_id in order to EXECUTED
        execQuery(mod_sell.str());

        // update buyer order amount
        double new_amount = sell_amount + buy_amount;
        stringstream update_buy_amount;
        update_buy_amount
            << "UPDATE ORDERS SET AMOUNT=" << to_string(new_amount)
            << " WHERE TRANS_ID=" << to_string(buy_id) << ";";
        execQuery(update_buy_amount.str());

        // add position to buyer
        changePosition(buy_info.acc_id, buy_info.sym, sell_pos);
        // add money to seller
        stringstream seller_add_balance;
        double new_balance = getAccBalance(sell_info.acc_id) + execution_price * sell_pos;
        updateBalance(sell_info.acc_id, new_balance);

        // if isBuy, refund
        if (isBuy)
        {
            double old_balance = getAccBalance(buy_info.acc_id);
            double refund = (buy_price - execution_price) * sell_pos;
            updateBalance(buy_info.acc_id, old_balance + refund);
        }

        // buyer wants more, match for buyer
        long match_id = matchForBuyer(buy_id);

        // if isBuy and match, executeOrder again
        if (isBuy && match_id != -1)
        {
            executeOrder(buy_id, match_id, true);
        }
        else
        {
            return;
        }
    }
}

void Database::handleOrder(long trans_id)
{
    double amount = getOrderAmount(trans_id);

    // mtx.lock();
    if (amount > 0)
    {
        // buy
        long match_id = matchForBuyer(trans_id);
        if (match_id == -1)
        {
            return;
        }
        else
        {
            executeOrder(trans_id, match_id, true);
        }
    }
    else
    {
        // sell
        long match_id = matchForSeller(trans_id);
        if (match_id == -1)
        {
            return;
        }
        else
        {
            executeOrder(match_id, trans_id, false);
        }
    }
    // mtx.unlock();
}

vector<Status> Database::query(long trans_id)
{
    vector<Status> vec;
    stringstream query_open;
    stringstream query_cancel;
    stringstream query_execute;

    work N0(*C);
    // query from open orders
    query_open << "SELECT AMOUNT FROM ORDERS WHERE TRANS_ID=" << to_string(trans_id)
               << " AND STATUS=" << N0.quote("OPEN") << ";";
    result R_open = N0.exec(query_open);
    N0.commit();

    for (result::const_iterator c = R_open.begin(); c != R_open.end(); ++c)
    {
        Status status_open = {0, c[0].as<double>(), 0, 0};
        vec.push_back(status_open);
    }

    work N1(*C);
    // query from canceled transactions
    query_cancel
        << "SELECT AMOUNT, CANCEL.TIME FROM CANCEL, ORDERS WHERE CANCEL.TRANS_ID=" << to_string(trans_id)
        << " AND CANCEL.TRANS_ID=ORDERS.TRANS_ID;";
    result R_cancel = N1.exec(query_cancel);
    N1.commit();
    for (result::const_iterator c = R_cancel.begin(); c != R_cancel.end(); ++c)
    {
        Status status_cancel = {1, c[0].as<double>(), 0, c[1].as<long>()};
        vec.push_back(status_cancel);
    }

    work N2(*C);
    // query from executed transactions
    query_execute << "SELECT AMOUNT, PRICE, TIME FROM EXECUTE WHERE TRANS_ID=" << to_string(trans_id) << ";";
    result R_execute = N2.exec(query_execute);
    N2.commit();
    for (result::const_iterator c = R_execute.begin(); c != R_execute.end(); ++c)
    {
        Status status_execute = {2, c[0].as<double>(), c[1].as<double>(), c[2].as<long>()};
        vec.push_back(status_execute);
    }

    return vec;
}

vector<Status> Database::cancel(long trans_id)
{
    vector<Status> vec;
    stringstream get_order;
    stringstream insert_cancel;
    stringstream modify_status;
    stringstream query_execute;
    double amount;
    double price;
    string sym;
    string acc_id;

    time_t t = time(NULL);

    nontransaction N0(*C);
    // cancel order
    get_order << "SELECT AMOUNT, SYMBOL, PRICE, ACCOUNT_ID FROM ORDERS WHERE TRANS_ID=" << to_string(trans_id)
              << " AND STATUS=" << N0.quote("OPEN") << " FOR UPDATE;";
    result R_get(N0.exec(get_order));
    
    N0.commit();

    if (R_get.size() == 0)
    {
        return vec;
    }

    for (result::const_iterator c = R_get.begin(); c != R_get.end(); ++c)
    {
        Status sta = {1, c[0].as<double>(), 0, (long)t};
        vec.push_back(sta);

        amount = c[0].as<double>();
        sym = c[1].as<string>();
        price = c[2].as<double>();
        acc_id = c[3].as<string>();
    }

    nontransaction N1(*C);
    // query from executed transactions
    query_execute << "SELECT AMOUNT, PRICE, TIME FROM EXECUTE WHERE TRANS_ID=" << to_string(trans_id) << ";";
    result R_execute(N1.exec(query_execute));
    N1.commit();

    for (result::const_iterator c = R_execute.begin(); c != R_execute.end(); ++c)
    {
        Status status_execute = {2, c[0].as<double>(), c[1].as<double>(), c[2].as<long>()};
        vec.push_back(status_execute);
    }

    work W(*C);
    // insert into CANCEL
    insert_cancel
        << "INSERT INTO CANCEL (TRANS_ID, TIME) VALUES ("
        << W.quote(trans_id) << ", " << to_string((long)t) << ");";

    // modify status
    modify_status
        << "UPDATE ORDERS SET STATUS=" << W.quote("CANCELED")
        << " WHERE TRANS_ID=" << to_string(trans_id) << ";";
    W.commit();

    execQuery(insert_cancel.str());
    execQuery(modify_status.str());
    
    
    // refund
    // get amount
    if(amount >0){
    // bigger than 0, add balance
        double balance = getAccBalance(acc_id);
        double new_balance = balance + amount * price;
        updateBalance(acc_id, new_balance);
    }else{
    // less than 0, add amount
        changePosition(acc_id, sym, -amount);
    }

    return vec;
}

void Database::createTables()
{

    // create ACCOUNT, POSITION, ORDERS, EXECUTE and CANCEL tables
    stringstream queryAccount;
    stringstream queryPosition;
    stringstream queryOrders;
    stringstream queryExecute;
    stringstream queryCancel;

    queryAccount
        << "CREATE TABLE ACCOUNT("
        << "ACCOUNT_ID VARCHAR(256),"
        << "BALANCE FLOAT(53),"
        << "PRIMARY KEY (ACCOUNT_ID));";

    queryPosition
        << "CREATE TABLE POSITION("
        << "ACCOUNT_ID VARCHAR(256),"
        << "SYMBOL VARCHAR(256),"
        << "AMOUNT FLOAT(53),"
        << "PRIMARY KEY (ACCOUNT_ID, SYMBOL),"
        << "CONSTRAINT UNIQUE_ACCOUNT_SYMBOL UNIQUE (ACCOUNT_ID, SYMBOL),"
        << "FOREIGN KEY (ACCOUNT_ID) REFERENCES ACCOUNT (ACCOUNT_ID) ON DELETE SET NULL ON UPDATE CASCADE);";

    queryOrders
        << "CREATE TABLE ORDERS("
        << "TRANS_ID BIGSERIAL,"
        << "ACCOUNT_ID VARCHAR(256),"
        << "SYMBOL VARCHAR(256),"
        << "AMOUNT FLOAT(53),"
        << "PRICE FLOAT(53),"
        << "TIME BIGINT,"
        << "STATUS VARCHAR(256),"
        << "PRIMARY KEY (TRANS_ID),"
        << "FOREIGN KEY (ACCOUNT_ID) REFERENCES ACCOUNT (ACCOUNT_ID) ON DELETE SET NULL ON UPDATE CASCADE);";

    queryCancel
        << "CREATE TABLE CANCEL("
        << "TRANS_ID BIGINT,"
        << "TIME BIGINT,"
        << "FOREIGN KEY (TRANS_ID) REFERENCES ORDERS (TRANS_ID) ON DELETE SET NULL ON UPDATE CASCADE);";

    queryExecute
        << "CREATE TABLE EXECUTE("
        << "TRANS_ID BIGINT,"
        << "AMOUNT FLOAT(53),"
        << "PRICE FLOAT(53),"
        << "TIME BIGINT,"
        << "FOREIGN KEY (TRANS_ID) REFERENCES ORDERS (TRANS_ID) ON DELETE SET NULL ON UPDATE CASCADE);";

    execQuery(queryAccount.str());
    execQuery(queryPosition.str());
    execQuery(queryOrders.str());
    execQuery(queryCancel.str());
    execQuery(queryExecute.str());
}

void Database::dropTables()
{
    string queryExecute = "DROP TABLE IF EXISTS EXECUTE CASCADE;";
    string queryCancel = "DROP TABLE IF EXISTS CANCEL CASCADE;";
    string queryOrders = "DROP TABLE IF EXISTS ORDERS CASCADE;";
    string queryPosition = "DROP TABLE IF EXISTS POSITION CASCADE;";
    string queryAccount = "DROP TABLE IF EXISTS ACCOUNT CASCADE;";

    execQuery(queryExecute);
    execQuery(queryCancel);
    execQuery(queryOrders);
    execQuery(queryPosition);
    execQuery(queryAccount);
}