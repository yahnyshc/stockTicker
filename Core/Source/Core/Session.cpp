#include <boost/program_options.hpp>
#include <json/json.h>
#include <json/reader.h>
#include "Session.hpp"

namespace po = boost::program_options;
using namespace web::websockets::client;

Session::Session(std::string fileName) {
    po::options_description config("Configuration");
    config.add_options()
        ("API_Token", po::value<std::string>()->default_value(""), "API Key assigned by Finnhub");
    config.add_options()
        ("Symbol_List", po::value<std::vector<std::string>>()->multitoken(), "List of Symbols to be subscribed");
    
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
        auto symbols = vm["Symbol_List"].as<std::vector<std::string>>();
        for (const auto& symbol : symbols) {
            std::cout << "Subscribing to the symbol " << symbol << std::endl;
            subscriptions_.push_back("{\"type\":\"subscribe\",\"symbol\":\"" + symbol + "\"}");
        }
    } else {
        std::cerr << "Symbol_List is not defined in the configuration file" << std::endl;
    }
}

void Session::subscribe() {
    Client_.connect(U("wss://ws.finnhub.io/?token=" + token_)).wait();
    
    for(auto subscription:subscriptions_) {
        websocket_outgoing_message msg;
        msg.set_utf8_message(subscription);
        Client_.send(msg).wait();
    }
}

void Session::run_forever() {
    auto next_update_time = 0L;
    while(true) {
        if(time(NULL) < next_update_time) continue;
        next_update_time = time(NULL) + 1;
        request_async().wait();
    }
}

pplx::task<void> Session::request_async() {
    std::string body_str;
    
    return Client_.receive().then([body_str](websocket_incoming_message ret_msg) {
        auto msg = ret_msg.extract_string().get();
        std::cout << "Received Message " << msg << "\n";
    });
}