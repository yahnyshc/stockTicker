#ifndef DATA_STORAGE_HPP
#define DATA_STORAGE_HPP

#include <pqxx/pqxx>
#include <limits>
#include <deque>

const std::string DB_NAME = "ticker";
const std::string DB_USER = "postgres";
const std::string DB_PASS = "postgres";
const std::string DB_TABLE = "ticker_history";

class DataStorage {

public:
    static DataStorage* getInstance();

    void connect();
    void savePrice(const std::string symbol, double price);
    std::deque<double> getPriceHistory(const std::string symbol, int period);
    int secondsSinceLastUpdate();
    double closedMarketPrice(const std::string symbol);
    double getLastPrice(const std::string symbol);
    void verifyConnection();

private:
    static DataStorage* instance_;   // The one, single instance
    DataStorage(); // private constructor
    ~DataStorage(); // private destructor
    DataStorage(const DataStorage&);
    DataStorage& operator=(const DataStorage&);

    std::unique_ptr<pqxx::connection> connection_;
};

#endif // DATA_STORAGE_HPP
