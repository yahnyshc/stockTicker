#include <json/json.h>
#include <json/reader.h>
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/streams.h>
#include <filesystem>
#include <iostream>
#include "Session.hpp"
#include "Core/Images/ImageManipulator.hpp"
#include "Core/Database/DataStorage.hpp"

using namespace web::websockets::client;
using namespace web::http::client;
namespace fs = std::filesystem;

Session::Session() {}

void Session::subscribe() {
    Client_.connect(U("wss://ws.finnhub.io/?token=" + c.get_token())).wait();
    
    for (const auto& symbol : c.get_symbols()) {
        std::cout << "Subscribing to the symbol " << symbol << std::endl;
        websocket_outgoing_message msg;
        msg.set_utf8_message("{\"type\":\"subscribe\",\"symbol\":\"" + symbol + "\"}");
        Client_.send(msg).wait();
    }
}

void Session::run_forever() {
    auto next_update_time = 0L;

    while(true) {
        if(time(NULL) < next_update_time) continue;
        next_update_time = time(NULL) + 1;
        std::string update = request_json_async().get();

        // Parse JSON
        Json::Value root;
        Json::Reader reader;
        bool parsingSuccessful = reader.parse(update, root);

        if (!parsingSuccessful) {
            std::cout << "Error parsing JSON" << std::endl;
            continue;
        }

        if (root["type"].asString() == "ping") {
            std::cout << "Received ping" << std::endl;
            continue;
        }

        if (root["type"].asString() == "trade") {
            std::cout << "Received trade" << std::endl;
            // Ensure data is an array
            if (root["data"].isArray()) {
                for (const auto& trade : root["data"]) {
                    std::string symbol = trade["s"].asString();
                    // price
                    double price = trade["p"].asDouble();

                    // save to database
                    d.save_price(symbol, price);
                }
            }
            continue;
        }

    }
}

pplx::task<std::string> Session::request_json_async() {
    std::string body_str;

    return Client_.receive().then([body_str](websocket_incoming_message ret_msg) {
        auto msg = ret_msg.extract_string().get();
        std::cout << "Received Message " << msg << "\n";
        return msg;
    });
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
            i.reduce(32, 32);
        }
    }
}
