#ifndef Session_HPP
#define Session_HPP

#include <cpprest/ws_client.h>
#include <set>
#include <memory>
#include "Core/Config.hpp"
#include "Core/Images/ImageManipulator.hpp"
#include "Core/Database/DataStorage.hpp"
#include "Core/Render/Renderer.hpp"
#include "Core/GlobalParams.hpp"

const std::string FINNHUB_URL = "wss://ws.finnhub.io/?token=";
const std::string LOGO_URL = "https://financialmodelingprep.com/image-stock/";

class Session : public std::enable_shared_from_this<Session>{

public:
    Session();

    void subscribe();
    void save_logos();
    void run_forever();
private:
    pplx::task<void> fetch_logo(const std::string& logo);
    void subscribe_to_symbol(const std::string& symbol);
    void process_message(const std::string& update);

    void reconnect();

    std::shared_ptr<web::websockets::client::websocket_callback_client> Client_ = std::make_shared<web::websockets::client::websocket_callback_client>();
    Config c = Config(CONFIG_FILE);
    DataStorage *d = DataStorage::getInstance();

    Renderer r = Renderer();

    std::string primary_symbol;

    std::unordered_map<std::string, double> latest_prices_;
};

#endif // Session_HPP