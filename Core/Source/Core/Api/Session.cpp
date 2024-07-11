#include <json/json.h>
#include <json/reader.h>
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/streams.h>
#include <filesystem>
#include <iostream>
#include "Session.hpp"


using namespace web::websockets::client;
using namespace web::http::client;
namespace fs = std::filesystem;

Session::Session() {}

void Session::subscribe() {
    Client_.connect(U("wss://ws.finnhub.io/?token=" + c.get_token())).wait();
    
    Client_.set_message_handler([=](websocket_incoming_message msg) {
        msg.extract_string().then([=](std::string msg) {
            std::cout << "Received Message: " << msg << std::endl;
            process_message(msg);
        }).wait();
    });

    Client_.set_close_handler([=](websocket_close_status close_status, const utility::string_t& reason, const std::error_code& error) {
        std::cout << "WebSocket Closed: " << reason << std::endl;
        subscribe();
    });

    for (const auto& symbol : c.get_symbols()) {
        subscribe_to_symbol(symbol);
    }
}

void Session::run_forever() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Session::subscribe_to_symbol(const std::string& symbol) {
    std::cout << "Subscribing to symbol " << symbol << std::endl;
    websocket_outgoing_message msg;
    msg.set_utf8_message("{\"type\":\"subscribe\",\"symbol\":\"" + symbol + "\"}");
    Client_.send(msg).wait();
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
        std::cout << "Received trade" << std::endl;
        // Ensure data is an array
        if (root["data"].isArray()) {
            for (const auto& trade : root["data"]) {
                std::string symbol = trade["s"].asString();

                if (parsed_symbols.find(symbol) != parsed_symbols.end()) continue;

                // price
                double price = trade["p"].asDouble();
                bool temporary_price = true;

                // update every 60 seconds
                if (d->seconds_since_last_update(symbol) >= 60){
                    // save to database
                    d->save_price(symbol, price);

                    temporary_price = false;

                    std::cout << "Updated price for symbol " << symbol << " to " << price << std::endl;
                }
                std::cout << "symbol: " << symbol << "\n\n";
                r.render_percentage(symbol, price);
                r.render_chart(symbol, price, temporary_price);
                parsed_symbols.insert(symbol);
            }
        }
        parsed_symbols.clear();
    }

}

pplx::task<void> Session::fetch_logo(const std::string& logo){
    std::string url = "https://financialmodelingprep.com/image-stock/"+logo+".png";
    std::cout << url << std::endl;
    http_client client(U(url));

    return client.request(web::http::methods::GET).then([=](web::http::http_response response) {
        if (response.status_code() == web::http::status_codes::OK) {
            // Save the response body (logo image) to a file
            Concurrency::streams::fstream::open_ostream("logos/"+logo+".png").then([=](Concurrency::streams::ostream output) {
                return response.body().read_to_end(output.streambuf());
            }).then([=](size_t) {
                std::cout << "Logo downloaded successfully.\n";
            }).wait();
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

        if ( ! fs::exists(symbol_logo) ){            
            fetch_logo(logo).wait();
            ImageManipulator i("logos/"+logo+".png");
            i.reduce(20, 20);
        }
    }
}
