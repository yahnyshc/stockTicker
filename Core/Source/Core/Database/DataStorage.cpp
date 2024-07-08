#include <iostream>
#include "DataStorage.hpp"

DataStorage::DataStorage(){
    // connect to postgres sql
    try {
        c = std::make_unique<pqxx::connection>("dbname = ticker user = postgres password = postgres \
            hostaddr = 127.0.0.1 port = 5432");
        if (c->is_open()) {
            std::cout << "Opened database successfully: " << c->dbname() << std::endl;
        } else {
            std::cout << "Can't open database" << std::endl;
        }
   } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
   }
}

DataStorage::~DataStorage(){
    c->disconnect();
}

void DataStorage::save_price(const std::string symbol, double price) {
    // save to database
    try {
        std::string sql = "INSERT INTO ticker_history VALUES ('" + symbol + "', " + std::to_string(price) + ");";

        /* Create a transactional object. */
        pqxx::work W(*c);
        
        /* Execute SQL query */
        W.exec( sql );
        W.commit();
        std::cout << "Saved price successfuly" << std::endl;
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}