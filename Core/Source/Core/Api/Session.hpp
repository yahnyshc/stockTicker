#ifndef SESSION_HPP
#define SESSION_HPP

#include <cpprest/ws_client.h>
#include <set>
#include <memory>
#include "Core/Config.hpp"
#include "Core/Images/ImageManipulator.hpp"
#include "Core/Database/DataStorage.hpp"
#include "Core/Render/Renderer.hpp"
#include "Core/GlobalParams.hpp"

using namespace web::websockets::client;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session();

    void chooseConfigAndSubscribe();
    void saveLogos();
    void runForever();

private:
    pplx::task<void> fetchLogo(const std::string& logo);
    void subscribeToSymbol(const std::string& symbol);
    void processMessage(const std::string& update);
    void priceUpdateCheck();
    void clearPriceHistory();
    void render(std::string symbol, double price, bool savePrice, bool fully);
    void primarySymbolSwitchCheck();
    void configUpdate(const std::string& config);
    void subscribe();
    void disconnect();
    void controllerSubscribe();
    void disconnectController();

    std::unique_ptr<websocket_callback_client> client_;
    std::unique_ptr<websocket_callback_client> controllerClient_;

    Config *config_ = Config::getInstance(CONFIG_FILE);

    DataStorage* dataStorage_ = DataStorage::getInstance();

    Renderer renderer_;

    std::unordered_map<std::string, double> latestPrices_;

    std::mutex priceConfigMutex;  // Mutex for price and config updates
};

#endif // SESSION_HPP