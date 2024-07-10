#ifndef Session_HPP
#define Session_HPP

#include <cpprest/ws_client.h>
#include <set>
#include "Config.hpp"
#include "Core/Images/ImageManipulator.hpp"
#include "Core/Database/DataStorage.hpp"
#include "Core/Render/Renderer.hpp"

class Session {

public:
    Session();

    void subscribe();
    void save_logos();
    void run_forever();

private:
    pplx::task<std::string> request_json_async();
    pplx::task<void> fetch_logo(const std::string& logo);
    std::string symbol_to_logo(const std::string& symbol);

    web::websockets::client::websocket_client Client_;
    Config c = Config("ws.cfg");
    DataStorage *d = DataStorage::getInstance();

    Renderer r = Renderer(43, 18);
};

#endif // Session_HPP