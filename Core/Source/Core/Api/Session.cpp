#include <json/json.h>
#include <json/reader.h>
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/streams.h>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <algorithm>

#include "Session.hpp"


using namespace web::websockets::client;
using namespace web::http::client;
namespace fs = std::filesystem;

volatile long long last_update_time = time(NULL);
volatile bool interrupt_received = false;
volatile bool reconnecting = false;

static void interrupt_handler(int signo) {
    interrupt_received = true;
    std::cout << "Interrupt signal received. Stopping the session." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

Session::Session() {
    signal(SIGTERM, interrupt_handler);
    signal(SIGINT, interrupt_handler);

    // initialize latest prices to MISSING_PRICE
    for (const auto& symbol : c.get_symbols()) 
        latest_prices_[symbol] = MISSING_PRICE;
}

void Session::subscribe() {
    std::cout << "Subscribing to symbols..." << std::endl;
    try {
        auto self = shared_from_this();
        Client_->connect(U(FINNHUB_URL + c.get_token())).wait();

        Client_->set_message_handler([self](websocket_incoming_message msg) {
            if ((!interrupt_received) && (self->r.get_matrix() != nullptr)) {
            if ((!interrupt_received) && (self->r.get_matrix() != nullptr)) {
                msg.extract_string().then([self](std::string msg) {
                    std::cout << "Received Message: " << msg << std::endl;
                    self->process_message(msg);
                }).wait();
            }
        });

        Client_->set_close_handler([self](websocket_close_status close_status, const utility::string_t& reason, const std::error_code& error) {
            std::cout << "WebSocket Closed: " << reason << std::endl;
            if ((!reconnecting) && (!interrupt_received) && (self->r.get_matrix() != nullptr)) {
            if ((!reconnecting) && (!interrupt_received) && (self->r.get_matrix() != nullptr)) {
                self->reconnect();
                std::cout << "Reconnected to server." << std::endl;
                std::cout << "Reconnected to server." << std::endl;
            }
        });

        for (const auto& symbol : c.get_symbols()) {
            subscribe_to_symbol(symbol);
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        reconnect();
    }
}

void Session::reconnect() {
    reconnecting = true;
    std::cout << "Reconnecting..." << std::endl;
    std::cout << "Closing the old client connection..." << std::endl;

    try {
        auto close_task = Client_->close();
        // Check periodically if the task is done, with a timeout of 10 seconds
        auto start_time = std::chrono::steady_clock::now();
        auto timeout = std::chrono::seconds(10);

        while (!close_task.is_done()) {
            if (std::chrono::steady_clock::now() - start_time > timeout) {
                std::cout << "Closing the WebSocket connection timed out." << std::endl;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (close_task.is_done()) {
            close_task.wait(); // Ensure any exceptions are thrown if the task completed
            std::cout << "Closed the old client connection." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception occurred while closing the WebSocket: " << e.what() << std::endl;
    }
    Client_.reset();
    std::cout << "Creating new client..." << std::endl;
    std::cout << "Closing the old client connection..." << std::endl;

    try {
        auto close_task = Client_->close();
        // Check periodically if the task is done, with a timeout of 10 seconds
        auto start_time = std::chrono::steady_clock::now();
        auto timeout = std::chrono::seconds(10);

        while (!close_task.is_done()) {
            if (std::chrono::steady_clock::now() - start_time > timeout) {
                std::cout << "Closing the WebSocket connection timed out." << std::endl;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (close_task.is_done()) {
            close_task.wait(); // Ensure any exceptions are thrown if the task completed
            std::cout << "Closed the old client connection." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception occurred while closing the WebSocket: " << e.what() << std::endl;
    }
    Client_.reset();
    std::cout << "Creating new client..." << std::endl;
    Client_ = std::make_shared<web::websockets::client::websocket_callback_client>();
    subscribe();
    reconnecting = false;
}

void Session::run_forever() {
    int primary_symbol_index = 0;
    
    // render primary logo chosen
    primary_symbol = c.get_symbols()[primary_symbol_index];
    r.render_entire_symbol(primary_symbol, latest_prices_[primary_symbol]);

    long long next_update_time = time(NULL) + SCROLLING_TIME;
    while (!interrupt_received) {
        // while we are reconnecting, simply spin and wait
        while( reconnecting ){
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // while we are reconnecting, simply spin and wait
        while( reconnecting ){
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        if(time(NULL) > next_update_time) {
            // move on to next subscription
            primary_symbol_index = (primary_symbol_index + 1) % c.get_symbols().size();
            primary_symbol = c.get_symbols()[primary_symbol_index];
            r.render_entire_symbol(primary_symbol, latest_prices_[primary_symbol]);
            next_update_time = time(NULL) + SCROLLING_TIME;
        }

        // if not trades received in last RECONNECTION_TRIGGER_TIME seconds, resubscribe
        if (time(NULL) > last_update_time + RECONNECTION_TRIGER_TIME) {
            reconnect();
            last_update_time = time(NULL);
        }
    }

    Client_->close().wait();
    Client_.reset();
    std::cout << "Session stopped" << std::endl;
}

void Session::subscribe_to_symbol(const std::string& symbol) {
    std::cout << "Subscribing to symbol " << symbol << std::endl;
    websocket_outgoing_message msg;
    msg.set_utf8_message("{\"type\":\"subscribe\",\"symbol\":\"" + symbol + "\"}");
    Client_->send(msg).wait();
}

void Session::process_message(const std::string& update) {
    // Parse JSON
    Json::Value root;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse(update, root);

    if (!parsingSuccessful) {
        std::cout << "Error parsing JSON" << std::endl;
        return;
    }

    if (root["type"].asString() == "ping") {
        std::cout << "Received ping" << std::endl;
        return;
    }

    std::set<std::string> parsed_symbols;
    if (root["type"].asString() == "trade") {
        last_update_time = time(NULL);
        std::cout << "Received trade" << std::endl;
        // Ensure data is an array
        if (root["data"].isArray()) {
            for (const auto& trade : root["data"]) {
                std::string symbol = trade["s"].asString();

                if (parsed_symbols.find(symbol) != parsed_symbols.end()) continue;

                // price
                double price = trade["p"].asDouble();
                if ( ! price ) continue;

                latest_prices_[symbol] = price;
                latest_prices_[symbol] = price;

                parsed_symbols.insert(symbol);
            }
        }
        parsed_symbols.clear();
    }

    static bool first_save = true;
    int seconds_since_last_update = d->seconds_since_last_update();
    if (seconds_since_last_update == std::numeric_limits<int>::max()){
        seconds_since_last_update = PRICE_TIME_INTERVAL;
    }
    bool temporary_price = ! (seconds_since_last_update >= PRICE_TIME_INTERVAL \
                            && (seconds_since_last_update % PRICE_TIME_INTERVAL) < ALLOWABLE_DISSYNCHRONIZATION_TIME);
    std::cout << "Seconds since last update: " << seconds_since_last_update << std::endl;
    for (const auto& symbol : c.get_symbols()) {
        double price = latest_prices_[symbol];
        if ( ! temporary_price ){
            if ( price != MISSING_PRICE ){
                // save to database
                d->save_price(symbol, price);
                std::cout << "Saved price: " << symbol << " " << price << std::endl;
            }
            latest_prices_[symbol] = MISSING_PRICE;
        }

        if ( ! temporary_price && ! first_save ){
            // push empty prices if more than one minute since last update
            for (int i = 0; i < (int)(seconds_since_last_update / PRICE_TIME_INTERVAL)-1; i++){
                r.update_chart(symbol, MISSING_PRICE, false, false);
            }
        }

        if (symbol == primary_symbol){                
            r.render_price(symbol, price);
            r.render_gain(symbol, price);
        }

        r.update_chart(symbol, price, temporary_price, symbol == primary_symbol);
    } 

    if ( ! temporary_price ){
        first_save = false;
    }
}

pplx::task<void> Session::fetch_logo(const std::string& logo){
    std::string url = LOGO_URL+logo+LOGO_EXT;
    http_client client(url);
    
    return client.request(web::http::methods::GET).then([=](web::http::http_response response) {
        if (response.status_code() == web::http::status_codes::OK) {
            // Save the response body (logo image) to a file
                try {
                    Concurrency::streams::fstream::open_ostream(LOGO_DIR+"/"+logo+LOGO_EXT).then([=](Concurrency::streams::ostream output) {
                        return response.body().read_to_end(output.streambuf());
                        } ).then([=](size_t) {
                        std::cout << "Logo downloaded successfully.\n";
                    }).wait();
                } catch(std::exception &e){
                    std::cerr << "Exception writing file: " << e.what() << "\n";	
                }
        } else {
            std::cout << "Failed to download logo. Status code: " << response.status_code() << "\n";
        }
    }); 
    
}

void Session::save_logos() {
    fs::path logos_path{LOGO_DIR};

    if ( ! fs::exists(logos_path) ) 
        fs::create_directory(logos_path);
    
    for (const auto& symbol : c.get_symbols()) {
        std::string logo = c.symbol_to_logo(symbol);
        std::string logo_path = LOGO_DIR + "/" + logo + LOGO_EXT;
        fs::path symbol_logo{logo_path};

        if ( ! fs::exists(symbol_logo) || cv::imread(logo_path).size().width != c.get_logo_size() ){     
            try{
                fetch_logo(logo).wait();
            }  catch(std::exception &e){
                std::cerr << "Logo does not exists on the website. Try adding 'Icon_Mappings' value to the config file.\n";
            }         
            
            if ( fs::exists(fs::path{LOGO_DIR + "/" + logo + LOGO_EXT}) ){
                ImageManipulator i(LOGO_DIR + "/" + logo + LOGO_EXT);
                i.reduce(c.get_logo_size(), c.get_logo_size());
            }
        }
    }
}
