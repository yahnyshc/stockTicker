#include <iostream>
#include <thread>
#include <chrono>
#include "DataStorage.hpp"

DataStorage* DataStorage::inst_ = NULL;

DataStorage* DataStorage::getInstance() {
   if (inst_ == NULL) {
      inst_ = new DataStorage();
   }
   return(inst_);
}

void DataStorage::connect() {
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

void DataStorage::verify_connection() {
    while (!c->is_open()) {
        try{
            connect();
            std::this_thread::sleep_for(std::chrono::seconds(3));
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }
}

DataStorage::DataStorage(){
    // connect to postgres sql
    connect();
}

DataStorage::~DataStorage(){
    c->disconnect();
}

void DataStorage::save_price(const std::string symbol, double price) {
    verify_connection();
    // save to database
    try {
        std::string sql = "INSERT INTO ticker_history VALUES ('" + symbol + "', " + std::to_string(price) + ");";

        /* Create a transactional object. */
        pqxx::work W(*c);
        
        /* Execute SQL query */
        W.exec( sql );
        W.commit();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

std::deque<double> DataStorage::get_price_history(const std::string symbol, int mPeriod) {
    verify_connection();
    std::deque <double> prices;
    prices.clear();
    
    // get price history from the database
    try {
        std::string sql = R"(
        WITH filled_times AS (
            SELECT generate_series(
                now() - interval ')" + std::to_string(mPeriod) + R"( minutes',  -- start time
                now(),                          -- end time (current time)
                interval '60 seconds'           -- interval between each timestamp
            ) AS time
        ),
        filled_data AS (
            SELECT ft.time,
                COALESCE(th.price, -1) AS price
            FROM filled_times ft
            LEFT JOIN ticker_history th ON th.time >= ft.time - interval '60 seconds' -- lower bound
                                        AND th.time < ft.time -- upper bound 
                        AND th.symbol = ')" + symbol + R"('
        )
        SELECT price
        FROM filled_data
        ORDER BY time
        LIMIT )" + std::to_string(mPeriod) + R"(;)";
        
        // Create a non-transactional object
        pqxx::nontransaction n(*c);

        // Execute SQL query
        pqxx::result res = n.exec(sql);
        
        // Print results
        for (auto row : res) {
            prices.push_back(std::stod(row[0].c_str()));
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return prices;
}

int DataStorage::seconds_since_last_update(const std::string symbol) {
    verify_connection();
    int seconds = std::numeric_limits<int>::max();
    try {
        std::string sql = "SELECT EXTRACT(EPOCH FROM (NOW() - time)) AS seconds_difference \
                        FROM ticker_history WHERE symbol = '" + symbol + "'\
                        ORDER BY time \
                        DESC LIMIT 1;";
        
        // Create a non-transactional object
        pqxx::nontransaction n(*c);

        // Execute SQL query
        pqxx::result res = n.exec(sql);
        
        // Print results
        for (auto row : res) {
            seconds = std::stoi(row[0].c_str());
        }   
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return seconds;
}

double DataStorage::get_last_price(const std::string symbol) {
    verify_connection();
    double price = 0.0;
    try {
        std::string sql = "SELECT price FROM ticker_history WHERE symbol = '" + symbol + "'\
                        ORDER BY time \
                        DESC LIMIT 1;";
        
        // Create a non-transactional object
        pqxx::nontransaction n(*c);    

        // Execute SQL query
        pqxx::result res = n.exec(sql);

        // Print results
        for (auto row : res) {
            price = std::stod(row[0].c_str());
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return price;
} 

double DataStorage::closed_market_price(const std::string symbol) {
    verify_connection();
    double price = 0.0;
    try {
        // sql to select price closest to 00:00.
        std::string sql = "WITH today_midnight AS ( \
                          SELECT DATE_TRUNC('day', NOW()) AS midnight \
                          ) \
                          SELECT price \
                          FROM ticker_history, today_midnight \
                          WHERE symbol = '" + symbol + "'\
                          ORDER BY ABS(EXTRACT(EPOCH FROM (time - today_midnight.midnight))) \
                          ASC LIMIT 1;";
        
        // Create a non-transactional object
        pqxx::nontransaction n(*c);

        // Execute SQL query
        pqxx::result res = n.exec(sql);

        // Print results
        for (auto row : res) {
            price = std::stod(row[0].c_str());
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return price;
}