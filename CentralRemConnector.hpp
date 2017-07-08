#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <pmt/pmt.h>
#include <vector>

using namespace pmt;

class CentralRemConnector{

private:
    std::string dbname;
    std::string user;
    std::string password;
    std::string addr;
    bool db_connection;
    pqxx::connection *db_handler;     
public:

    CentralRemConnector(std::string dbname,std::string user,std::string password, std::string addr);
    ~CentralRemConnector();
    bool connect();
    int queryDB();
    void analyze(const char * recv_buffer, int buffer_len);
    std::string insert(pmt::pmt_t dict, std::string table);
    std::string update(pmt::pmt_t dict, pmt::pmt_t conditions, std::string table);
};