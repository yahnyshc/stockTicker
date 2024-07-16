#ifndef Renderer_HPP
#define Renderer_HPP

#include "led-matrix.h"
#include "graphics.h"
#include <algorithm>
#include <iostream>
#include <deque>
#include <cmath>
#include <unordered_map>
#include <iomanip>
#include <sstream>
#include <cmath>
#include "Core/Database/DataStorage.hpp"
#include "Core/Api/Config.hpp"
#include <cstdlib> // for rand() and srand()
#include <ctime>   // for time()
#include <unistd.h>  // for sleep

class Renderer {
public:
    Renderer();
    ~Renderer();

    void update_chart(std::string symbol, double last_price, bool temporary_price, bool to_render);
    void render_gain(std::string symbol, double last_price);
    void render_logo(std::string logo, int size);
    void render_symbol(std::string symbol);
    void render_price(double last_price);
    rgb_matrix::RGBMatrix* get_matrix();
private:
    std::unordered_map<std::string, double> closed_market_prices_;
    
    std::unordered_map<std::string, std::deque<double>> past_charts_;

    DataStorage *d = DataStorage::getInstance();

    void fetch_past_chart(std::string symbol, int chart_width);

    rgb_matrix::RGBMatrix* matrix;
    rgb_matrix::RGBMatrix::Options matrix_options;

    Config c = Config("ws.cfg");

    bool logo_rendered = c.get_bool_render_logos();
};

#endif // Renderer_HPP
