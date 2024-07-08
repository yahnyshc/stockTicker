#ifndef DataStorage_HPP
#define DataStorage_HPP

#include <pqxx/pqxx> 

class DataStorage {

public:
    DataStorage();
    ~DataStorage();

    void save_price(const std::string symbol, double price);

private:
    std::unique_ptr<pqxx::connection> c;
};

#endif // DataStorage_HPP