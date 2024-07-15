#ifndef DataStorage_HPP
#define DataStorage_HPP

#include <pqxx/pqxx> 
#include <limits>
#include <deque>

class DataStorage {

public:
    static DataStorage* getInstance();

    void connect();
    void save_price(const std::string symbol, double price);
    std::deque<double> get_price_history(const std::string symbol, int mPeriod);
    int seconds_since_last_update(const std::string symbol);
    double closed_market_price(const std::string symbol);
    double get_last_price(const std::string symbol);
    void verify_connection();

private:
    static DataStorage* inst_;   // The one, single instance
    DataStorage(); // private constructor
    ~DataStorage(); // private destructor
    DataStorage(const DataStorage&);
    DataStorage& operator=(const DataStorage&);

    std::unique_ptr<pqxx::connection> c;
};



#endif // DataStorage_HPP