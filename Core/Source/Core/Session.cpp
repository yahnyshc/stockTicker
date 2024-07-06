#include <boost/program_options.hpp>
#include <json/json.h>
#include <json/reader.h>
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/streams.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "Session.hpp"
#include "ImageManipulator.hpp"

namespace po = boost::program_options;
using namespace web::websockets::client;
using namespace web::http::client;
namespace fs = std::filesystem;

Session::Session(std::string fileName) {
    po::options_description config("Configuration");
    config.add_options()
        ("API_Token", po::value<std::string>()->default_value(""), "API Key assigned by Finnhub");
    config.add_options()
        ("Symbol_List", po::value<std::vector<std::string>>()->multitoken(), "List of Symbols to be subscribed");
    config.add_options()
        ("Icon_Mapping", po::value<std::vector<std::string>>()->multitoken(), "Symbol to Icon name translation");
    
    boost::program_options::variables_map vm;

    std::ifstream ifs(fileName);
    if (ifs) {
        boost::program_options::store(boost::program_options::parse_config_file(ifs, config), vm);
        boost::program_options::notify(vm);
    } else {
        std::cerr << "Could not open configuration file: " << fileName << std::endl;
        return;
    }
    
    if (vm.count("API_Token")) {
        token_ = vm["API_Token"].as<std::string>();
    } else {
        std::cerr << "API_Token is not defined in the configuration file" << std::endl;
        return;
    }

    if (vm.count("Symbol_List")) {
        symbols_ = vm["Symbol_List"].as<std::vector<std::string>>();
    } else {
        std::cerr << "Symbol_List is not defined in the configuration file" << std::endl;
    }

    if (vm.count("Icon_Mapping")) {
        icon_mappings_ = vm["Icon_Mapping"].as<std::vector<std::string>>();
    } else {
        std::cerr << "Icon_Mapping is not defined in the configuration file" << std::endl;
    }
}

void Session::subscribe() {
    Client_.connect(U("wss://ws.finnhub.io/?token=" + token_)).wait();
    
    for (const auto& symbol : symbols_) {
        std::cout << "Subscribing to the symbol " << symbol << std::endl;
        websocket_outgoing_message msg;
        msg.set_utf8_message("{\"type\":\"subscribe\",\"symbol\":\"" + symbol + "\"}");
        Client_.send(msg).wait();
    }
}

void Session::get_icons() {
    fs::path logos_path{"logos"};

    if ( ! fs::exists(logos_path) ) 
        fs::create_directory(logos_path);
    
    for (const auto& symbol : symbols_) {
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

void Session::run_forever() {
    auto next_update_time = 0L;

    while(true) {
        if(time(NULL) < next_update_time) continue;
        next_update_time = time(NULL) + 1;
        request_json_async().wait();
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
    std::string delimiter = " -> ";
    std::string logo_identifier = symbol;

    for ( std::string& mapping : icon_mappings_ ){
        size_t pos = mapping.find(delimiter);
        std::string token = mapping.substr(0, pos);
        if (token.compare(symbol) == 0)
            mapping.erase(0, pos + delimiter.length());
            logo_identifier = mapping;
    }
    
    return logo_identifier;
}