#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include "DataStorage.hpp"
#include "Core/GlobalParams.hpp"

DataStorage* DataStorage::instance_ = nullptr;
std::mutex dbMutex;

// singleton
DataStorage* DataStorage::getInstance() {
   if (instance_ == nullptr) {
      instance_ = new DataStorage();
   }
   return instance_;
}

DataStorage::DataStorage(){
    // connect to PostgreSQL
    connect();
}

DataStorage::~DataStorage(){
    connection_->disconnect();
}

void DataStorage::connect() {
    try {
        connection_ = std::make_unique<pqxx::connection>("dbname=" + DB_NAME + " user=" + DB_USER + 
            " password=" + DB_PASS + 
            " hostaddr=127.0.0.1 port=5432");
        if (connection_->is_open()) {
            std::cout << "Opened database successfully: " << connection_->dbname() << std::endl;
        } else {
            std::cout << "Can't open database" << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

void DataStorage::verifyConnection() {
    while (!connection_->is_open()) {
        try {
            connect();
            std::this_thread::sleep_for(std::chrono::seconds(3));
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }
}

void DataStorage::savePrice(const std::string symbol, double price) {
    verifyConnection();
    // save to database
    try {
        std::string sql = "INSERT INTO " + DB_TABLE + " VALUES ('" + symbol + "', " + std::to_string(price) + ");";

        /* Create a transactional object. */
        std::lock_guard<std::mutex> lock(dbMutex);
        pqxx::work W(*connection_);
        
        /* Execute SQL query */
        W.exec(sql);
        W.commit();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

std::deque<double> DataStorage::getPriceHistory(const std::string symbol, int period) {
    verifyConnection();
    std::deque<double> prices;
    prices.clear();
    
    // get price history from the database
    try {
        std::string sql = R"(
        WITH filled_times AS (
            SELECT generate_series(
                now() - interval ')" + std::to_string(period) + R"( minutes',  -- start time
                now(),                          -- end time (current time)
                interval ')" + std::to_string(PRICE_TIME_INTERVAL) + R"( seconds'           -- interval between each timestamp
            ) AS time
        ),
        filled_data AS (
            SELECT ft.time,
                COALESCE(th.price, )" + std::to_string(MISSING_PRICE) + R"() AS price
            FROM filled_times ft
            LEFT JOIN )" + DB_TABLE + R"( th ON th.time >= ft.time -- lower bound
                                        AND th.time < ft.time + interval ')" + std::to_string(PRICE_TIME_INTERVAL) + R"( seconds' -- upper bound 
                        AND th.symbol = ')" + symbol + R"('
        )
        SELECT price
        FROM filled_data
        ORDER BY time
        LIMIT )" + std::to_string(period) + R"(;)";
        
        // Create a non-transactional object
        std::lock_guard<std::mutex> lock(dbMutex);
        pqxx::nontransaction n(*connection_);

        // Execute SQL query
        pqxx::result res = n.exec(sql);
        
        // Process results
        for (auto row : res) {
            prices.push_back(std::stod(row[0].c_str()));
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return prices;
}

int DataStorage::secondsSinceLastUpdate() {
    verifyConnection();
    int seconds = std::numeric_limits<int>::max();
    try {
        std::string sql = "SELECT EXTRACT(EPOCH FROM (NOW() - MAX(time))) \
                           FROM " + DB_TABLE + ";";
        
        // Create a non-transactional object
        std::lock_guard<std::mutex> lock(dbMutex);
        pqxx::nontransaction n(*connection_);

        // Execute SQL query
        pqxx::result res = n.exec(sql);
        
        // Process results
        for (auto row : res) {
            seconds = std::stoi(row[0].c_str());
        }   
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return seconds;
}

double DataStorage::getLastPrice(const std::string symbol) {
    verifyConnection();
    double price = ZERO_PRICE;
    try {
        std::string sql = "SELECT price FROM " + DB_TABLE + " WHERE symbol = '" + symbol + "'\
                        ORDER BY time \
                        DESC LIMIT 1;";
        
        // Create a non-transactional object
        std::lock_guard<std::mutex> lock(dbMutex);
        pqxx::nontransaction n(*connection_);    

        // Execute SQL query
        pqxx::result res = n.exec(sql);

        // Process results
        for (auto row : res) {
            price = std::stod(row[0].c_str());
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return price;
} 

double DataStorage::closedMarketPrice(const std::string symbol) {
    verifyConnection();
    double price = ZERO_PRICE;
    try {
        // SQL to select price closest to 00:00.
        std::string sql = "WITH today_midnight AS ( \
                          SELECT DATE_TRUNC('day', NOW()) AS midnight \
                          ) \
                          SELECT price \
                          FROM " + DB_TABLE + ", today_midnight \
                          WHERE symbol = '" + symbol + "'\
                          ORDER BY ABS(EXTRACT(EPOCH FROM (time - today_midnight.midnight))) \
                          ASC LIMIT 1;";
        
        // Create a non-transactional object
        std::lock_guard<std::mutex> lock(dbMutex);
        pqxx::nontransaction n(*connection_);

        // Execute SQL query
        pqxx::result res = n.exec(sql);

        // Process results
        for (auto row : res) {
            price = std::stod(row[0].c_str());
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return price;
}
