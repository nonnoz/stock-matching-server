#include <iostream>
#include <pqxx/pqxx>

#define OPEN 0
#define CANCELED 1
#define EXECUTED 2

using namespace std;
using namespace pqxx;

struct Status
{
    // status 0: open
    // status 1: cancel
    // status 2: execute
    int status;
    double amount;
    double price;
    long time;
};

struct Order
{
    string acc_id;
    string sym;
    double amount;
    double price;
    long time;
};

class Database
{
private:
    connection *C;
    
    bool positionExist(string acc_id, string sym);
    bool createPosition(string acc_id, string sym, double amount);
    double getPositionAmount(string acc_id, string sym);
    void updateBalance(string acc_id, double balance);
    void updatePosition(string acc_id, string sym, double amount);
    // double getOrderPrice(long trans_id);
    double getOrderAmount(long trans_id);
    void executeOrder(long buy_id, long sell_id, bool isBuy);
    Order getOrderInfo(long trans_id);
    double getAccBalance(string acc_id);
    void execQuery(string query);


public:
    void connect(string db_param);

    void dropTables();
    void createTables();

    bool accountExist(string acc_id);
    bool createAccount(string acc_id, double balance);
    bool checkAccTrans(string acc_id, long trans_id);

    bool transExist(long trans_id);

    bool changePosition(string acc_id, string sym, double amount);

    bool buyValid(string acc_id, double amount, double price);
    bool sellValid(string acc_id, string sym, double amount);

    long createOrder(string acc_id, string sym, double amount, double price);
    long matchForBuyer(long trans_id);
    long matchForSeller(long trans_id);
    void handleOrder(long trans_id);

    vector<Status> query(long trans_id);
    vector<Status> cancel(long trans_id);
};