#include <json/json.h>
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/streams.h>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <csignal>
#include <mutex>
#include "Session.hpp"

using namespace web::websockets::client;
using namespace web::http::client;
namespace fs = std::filesystem;

namespace {
    // Constants
    const std::string FINNHUB_URL = "wss://ws.finnhub.io/?token=";
    const std::string LOGO_URL = "https://financialmodelingprep.com/image-stock/";
    const std::string CONTROL_URL = "wss://backend.stock-ticker-remote.link/ws?token="; 

    // Global variables
    volatile long long lastUpdateTime = time(nullptr);
    volatile bool interruptReceived = false;
    long long nextSwitchTime = 0;

    bool receivedFirstUpdate = false;

    bool connected = false;
    bool updatingConfig = false;

    std::mutex priceConfigMutex;  // Mutex for price and config updates
}

static void interruptHandler(int signo) {
    interruptReceived = true;
    std::cout << "Interrupt signal received. Stopping the session." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

Session::Session() {
    std::signal(SIGTERM, interruptHandler);
    std::signal(SIGINT, interruptHandler);

    // Initialize latest prices to MISSING_PRICE
    for (const auto& symbol : config_->getApiSubsList()) {
        latestPrices_[symbol] = MISSING_PRICE;
    }
}

void Session::chooseConfigAndSubscribe() {
    if (!config_->getControlToken().empty()) {
        controllerSubscribe();
    } else {
        subscribe();
        saveLogos();
    }
}

void Session::subscribe() {
    try {
        std::cout << "Creating new client..." << std::endl;
        client_ = std::make_unique<websocket_callback_client>();
    
        client_->connect(U(FINNHUB_URL + config_->getToken())).wait();
        connected = true;

        client_->set_message_handler([this](websocket_incoming_message msg) {
            if (!interruptReceived) {
                msg.extract_string().then([this](std::string msg) {
                    std::cout << "Received Message: " << msg << std::endl;
                    processMessage(msg);
                }).wait();
            }
        });

        client_->set_close_handler([this](websocket_close_status closeStatus, const utility::string_t& reason, const std::error_code& error) {
            if (!reason.empty()) {
                std::cout << "WebSocket Closed: " << reason << std::endl;
            } else {
                std::cout << "WebSocket Closed with no reason provided." << std::endl;
            }
            connected = false;
        });

        std::cout << "Subscribing to symbols..." << std::endl;
        for (const auto& symbol : config_->getApiSubsList()) {
            subscribeToSymbol(symbol);
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

void Session::disconnect() {
    std::cout << "Disconnecting..." << std::endl;
    std::cout << "Closing the old client connection..." << std::endl;

    if (client_) {
        client_->close().wait(); // Use wait to ensure close operation completes
        client_.reset();
        std::cout << "Closed the old client connection..." << std::endl;
    }

    connected = false;
}

std::vector<std::string> jsonArrayToVector(const Json::Value& jsonArray) {
    std::vector<std::string> vec;
    for (const auto& item : jsonArray) {
        if (item.isString()) {
            vec.push_back(item.asString());
        }
    }
    return vec;
}

void Session::configUpdate(const std::string& config) {
    std::lock_guard<std::mutex> lock(priceConfigMutex); 

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(config, root) || root["type"].asString() != "config") {
        std::cout << "Error parsing JSON or incorrect type" << std::endl;
        return;
    }

    std::vector<std::string> subsList = jsonArrayToVector(root["subs"]);
    std::vector<std::string> apiSubsList = jsonArrayToVector(root["api_names"]);
    std::vector<std::string> logoSubsList = jsonArrayToVector(root["logo_names"]);

    std::cout << "Subscrions updated." << std::endl;
    if (apiSubsList.empty()) {
        std::cout << "No subscriptions found." << std::endl;
        return;
    }

    static bool firstRun = true;
    static int configId;

    bool updateSubs = firstRun || 
        config_->getSubsList() != subsList ||
        config_->getApiSubsList() != apiSubsList;

    if ( updateSubs ) {
        disconnect();
        renderer_.clearPastCharts();
        if (configId != root["id"].asInt()) {
            currentSymbolIndex_ = -1;
            nextSwitchTime = 0;
            configId = root["id"].asInt();
        }
        config_->setSubsList(subsList);
        config_->setApiSubsList(apiSubsList);
        
        for (const auto& symbol : config_->getApiSubsList()) {
            latestPrices_[symbol] = MISSING_PRICE;
        }
    }
    
    if (firstRun || config_->getSwitchTime() != root["switch_time"].asInt()) {
        config_->setSwitchTime(root["switch_time"].asInt());
    }

    bool updateLogos = firstRun || config_->getLogoSubsList() != logoSubsList;
    if ( updateLogos ){
        config_->setLogoSubsList(logoSubsList);
        saveLogos();
    }

    if (updateSubs) {
        subscribe();
        updatingConfig = false;
    }

    if (firstRun) {
        firstRun = false;
    }
}

void Session::controllerSubscribe() {
    std::cout << "Subscribing controller app..." << std::endl;
    try {
        // websocket_client_config config;
        // config.set_validate_certificates(false); // Disable certificate validation
        controllerClient_ = std::make_unique<websocket_callback_client>();

        controllerClient_->set_message_handler([this](websocket_incoming_message msg) {
            if (!interruptReceived) {
                msg.extract_string().then([this](std::string body) {
                    std::cout << "Received message: " << body << std::endl;
                    configUpdate(body);
                }).wait();
            }
        });

        controllerClient_->set_close_handler([](websocket_close_status status, const utility::string_t& reason, const std::error_code& error) {
            std::cout << "Connection closed: " << reason << std::endl;
        });

        std::string uri = CONTROL_URL + config_->getControlToken();
        controllerClient_->connect(U(uri)).then([uri] {
            std::cout << "Connecting to: " << U(uri) << std::endl;
        }).wait();
    } catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
    }
}

void Session::priceUpdateCheck() {
    if (!receivedFirstUpdate) return;

    int secondsSinceLastUpdate = dataStorage_->secondsSinceLastUpdate();
    if (secondsSinceLastUpdate == std::numeric_limits<int>::max()) {
        secondsSinceLastUpdate = PRICE_TIME_INTERVAL;
    }
    bool temporaryPrice = !(secondsSinceLastUpdate >= PRICE_TIME_INTERVAL &&
                        (secondsSinceLastUpdate % PRICE_TIME_INTERVAL) < ALLOWABLE_DISSYNCHRONIZATION_TIME);

    std::cout << "Seconds since last update: " << secondsSinceLastUpdate << std::endl;
    for (const auto& symbol : config_->getApiSubsList()) {
        double price = latestPrices_[symbol];
        if (!temporaryPrice) {
            if (price != MISSING_PRICE) {
                dataStorage_->savePrice(symbol, price);
                std::cout << "Saved price: " << symbol << " " << price << std::endl;
            }
            latestPrices_[symbol] = MISSING_PRICE;
        }
        render(symbol, price, temporaryPrice, false);
    }
}

void Session::runForever() {
    // If using controller api, spin and wait for config update:
    while(config_->getApiSubsList().size() == 0 || config_->getLogoSubsList().size() == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    while (!interruptReceived) {
        if (priceConfigMutex.try_lock()) {
            if (!connected) {
                std::cout << "Client unexpectedly disconnected. Reconnecting..." << std::endl;
                disconnect();
                subscribe();
            }

            priceUpdateCheck();
            
            primarySymbolSwitchCheck();

            priceConfigMutex.unlock();
        } 
    }

    if (client_) {
        client_->close().wait(); // Ensure the client closes gracefully
    }
    std::cout << "Session stopped" << std::endl;
}

void Session::primarySymbolSwitchCheck() {
    if (time(nullptr) > nextSwitchTime) {
        currentSymbolIndex_ = (currentSymbolIndex_ + 1) % config_->getApiSubsList().size();
        std::string symbol = config_->getApiSubsList()[currentSymbolIndex_];
        double price = latestPrices_[config_->getApiSubsList()[currentSymbolIndex_]];
        render(symbol, price, true, true);
        nextSwitchTime = time(nullptr) + config_->getSwitchTime();
    }
}

void Session::render(std::string symbol, double price, bool temporaryPrice, bool fully){
    if (fully) {
        renderer_.renderEntireSymbol(currentSymbolIndex_, latestPrices_[config_->getApiSubsList()[currentSymbolIndex_]]);
    } else {
        bool isPrimarySymbol = config_->getApiSubsList()[currentSymbolIndex_] == symbol;
        if (isPrimarySymbol) {
            renderer_.renderPrice(symbol, price);
            renderer_.renderGain(symbol, price);
        }
        renderer_.updateChart(symbol, price, temporaryPrice, isPrimarySymbol);
    }
}

void Session::subscribeToSymbol(const std::string& symbol) {
    std::cout << "Subscribing to symbol " << symbol << std::endl;
    websocket_outgoing_message msg;
    msg.set_utf8_message("{\"type\":\"subscribe\",\"symbol\":\"" + symbol + "\"}");
    client_->send(msg).wait();
}

void Session::processMessage(const std::string& update) {
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(update, root)) {
        std::cout << "Error parsing JSON" << std::endl;
        return;
    }

    if (root["type"].asString() == "ping") {
        std::cout << "Received ping" << std::endl;
        return;
    }

    if (root["type"].asString() == "trade") {
        lastUpdateTime = time(nullptr);
        std::cout << "Received trade" << std::endl;

        std::set<std::string> parsedSymbols;
        if (root["data"].isArray()) {
            for (const auto& trade : root["data"]) {
                std::string symbol = trade["s"].asString();
                if (parsedSymbols.find(symbol) != parsedSymbols.end()) continue;

                double price = trade["p"].asDouble();
                if (price == 0) continue;

                latestPrices_[symbol] = price;
                parsedSymbols.insert(symbol);
            }
        }
        parsedSymbols.clear();

        if (!receivedFirstUpdate) receivedFirstUpdate = true;
    }
}

pplx::task<void> Session::fetchLogo(const std::string& logo) {
    std::string url = LOGO_URL + logo + LOGO_EXT;
    http_client client(url);

    return client.request(web::http::methods::GET).then([logo](web::http::http_response response) {
        if (response.status_code() == web::http::status_codes::OK) {
            try {
                Concurrency::streams::fstream::open_ostream(LOGO_DIR + "/" + logo + LOGO_EXT).then([response](Concurrency::streams::ostream output) {
                    return response.body().read_to_end(output.streambuf());
                }).then([](size_t) {
                    std::cout << "Logo downloaded successfully.\n";
                }).wait();
            } catch (const std::exception& e) {
                std::cerr << "Exception writing file: " << e.what() << "\n";    
            }
        } else {
            std::cout << "Failed to download logo. Status code: " << response.status_code() << "\n";
        }
    });
}

void Session::saveLogos() {
    fs::path logosPath{LOGO_DIR};

    if (!fs::exists(logosPath)) {
        fs::create_directory(logosPath);
    }

    for (const auto& logo : config_->getLogoSubsList()) {
        if (logo.empty()) continue;

        std::string logoPath = LOGO_DIR + "/" + logo + LOGO_EXT;
        fs::path symbolLogo{logoPath};

        if (!fs::exists(symbolLogo) || cv::imread(logoPath).size().width != config_->getLogoSize()) {
            try {
                fetchLogo(logo).wait();
            } catch (const std::exception& e) {
                std::cerr << "Logo does not exist on the website. Try adding 'Logo_Subs_list' to the config file.\n";
            }

            if (fs::exists(fs::path{LOGO_DIR + "/" + logo + LOGO_EXT})) {
                ImageManipulator imgManipulator(LOGO_DIR + "/" + logo + LOGO_EXT);
                imgManipulator.reduce(config_->getLogoSize(), config_->getLogoSize());
            }
        }
    }
}