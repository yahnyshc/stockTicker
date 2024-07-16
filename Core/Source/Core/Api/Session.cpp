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

#include "Session.hpp"


using namespace web::websockets::client;
using namespace web::http::client;
namespace fs = std::filesystem;

volatile long long last_update_time = time(NULL);
volatile bool interrupt_received = false;
volatile bool reconnecting = false;

static void InterruptHandler(int signo) {
    interrupt_received = true;
    std::cout << "Interrupt signal received. Stopping the session." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

Session::Session() {}

void Session::subscribe() {
    std::cout << "Subscribing to symbols..." << std::endl;
    try {
        auto self = shared_from_this();
        Client_->connect(U("wss://ws.finnhub.io/?token=" + c.get_token())).wait();

        Client_->set_message_handler([self](websocket_incoming_message msg) {
            if (!reconnecting && !interrupt_received && self->r.get_matrix() != nullptr) {
                msg.extract_string().then([self](std::string msg) {
                    std::cout << "Received Message: " << msg << std::endl;
                    self->process_message(msg);
                }).wait();
            }
        });

        Client_->set_close_handler([self](websocket_close_status close_status, const utility::string_t& reason, const std::error_code& error) {
            std::cout << "WebSocket Closed: " << reason << std::endl;
            if (!reconnecting && !interrupt_received && self->r.get_matrix() != nullptr) {
                self->reconnect();
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
    Client_->close().wait();
    Client_ = std::make_shared<web::websockets::client::websocket_callback_client>();
    subscribe();
    reconnecting = false;
}

void Session::render_primary_symbol() {
    r.get_matrix()->Fill(0, 0, 0);
    r.render_logo("logos/"+symbol_to_logo(primary_symbol)+".png", c.get_logo_size());
    r.render_symbol(symbol_to_logo(primary_symbol));
    double last_price = d->get_last_price(primary_symbol);
    r.render_price(last_price);
    r.render_gain(primary_symbol, last_price);
    r.update_chart(primary_symbol, last_price, true, true);
}


void Session::run_forever() {
    int primary_symbol_index = 0;

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);
    
    // render primary logo chosen
    primary_symbol = c.get_symbols()[primary_symbol_index];
    render_primary_symbol();

    long long next_update_time = time(NULL) + 5;
    while (!interrupt_received) {
        if(time(NULL) > next_update_time) {
            // move on to next subscription
            primary_symbol_index = (primary_symbol_index + 1) % c.get_symbols().size();
            primary_symbol = c.get_symbols()[primary_symbol_index];
            render_primary_symbol();
            next_update_time = time(NULL) + 5;
        }
        // if not trades received in last 30 seconds, resubscribe
        if (time(NULL) > last_update_time + 30) {
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

                bool temporary_price = true;

                // update every 60 seconds
                if (d->seconds_since_last_update(symbol) >= 60){
                    // save to database
                    d->save_price(symbol, price);

                    temporary_price = false;

                    std::cout << "Updated price for symbol " << symbol << " to " << price << std::endl;
                }                

                if (symbol == primary_symbol){
                    std::cout << "symbol: " << symbol << "\n\n";
                    r.render_price(price);
                    r.render_gain(symbol, price);
                }
                r.update_chart(symbol, price, temporary_price, symbol == primary_symbol);

                parsed_symbols.insert(symbol);
            }
        }
        parsed_symbols.clear();
    }

}

pplx::task<void> Session::fetch_logo(const std::string& logo){
    std::string url = "https://financialmodelingprep.com/image-stock/"+logo+".png";
    http_client client(url);
    
    return client.request(web::http::methods::GET).then([=](web::http::http_response response) {
        if (response.status_code() == web::http::status_codes::OK) {
            // Save the response body (logo image) to a file
                try {
                    Concurrency::streams::fstream::open_ostream("logos/"+logo+".png").then([=](Concurrency::streams::ostream output) {
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

std::string Session::symbol_to_logo(const std::string& symbol){
    std::string logo_identifier = symbol;

    for ( std::string mapping : c.get_icon_mappings() ){
        std::string delimiter = " -> ";
        size_t pos = mapping.find(delimiter);
        std::string token = mapping.substr(0, pos);
        if (token == symbol){
            mapping.erase(0, pos + delimiter.length());
            logo_identifier = mapping;
        }
    }
    return logo_identifier;
}

void Session::save_logos() {
    fs::path logos_path{"logos"};

    if ( ! fs::exists(logos_path) ) 
        fs::create_directory(logos_path);
    
    for (const auto& symbol : c.get_symbols()) {
        std::string logo = symbol_to_logo(symbol);
        std::string logo_path = "logos/" + logo + ".png";
        fs::path symbol_logo{logo_path};

        if ( ! fs::exists(symbol_logo) || cv::imread(logo_path).size().width != c.get_logo_size() ){     
            try{
                fetch_logo(logo).wait();
            }  catch(std::exception &e){
                std::cerr << "Logo does not exists on the website. Try adding Icon_Mappings value to the config file.\n";
            }         
            
            if ( fs::exists(fs::path{"logos/"+logo+".png"}) ){
                ImageManipulator i("logos/"+logo+".png");
                i.reduce(c.get_logo_size(), c.get_logo_size());
            }
        }
    }
}
