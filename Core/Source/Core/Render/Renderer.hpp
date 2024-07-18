#ifndef Renderer_HPP
#define Renderer_HPP

#include "led-matrix.h"
#include "graphics.h"
#include <algorithm>
#include <iostream>
#include <deque>
#include <unordered_map>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <cstdlib> // for rand() and srand()
#include <ctime>   // for time()
#include <unistd.h>  // for sleep
#include "Core/Database/DataStorage.hpp"
#include "Core/Config.hpp"
#include "Core/GlobalParams.hpp"

#define MATRIX_WIDTH 64
#define GPIO_SLOWDOWN 4
const std::string GPIO_MAPPING = "adafruit-hat";
const std::string RGB_SEQUENCE = "RBG";

#define LOGO_CHART_GAP 1
#define SYMBOL_LEFT_SPACING 2
#define SYMBOL_TOP_SPACING 1

#define PERCENTAGE_PRECISION 2

#define PERCENTAGE_FONT_WIDTH 4
#define PERCENTAGE_FONT_HEIGHT 6
#define PRICE_FONT_WIDTH 4
#define PRICE_FONT_HEIGHT 6
#define SYMBOL_FONT_WIDTH 5
#define SYMBOL_FONT_HEIGHT 7

const int chart_base_RGB[3] = {0, 140, 0};
const int chart_top_RGB[3] = {0, 255, 90};   

class Renderer {
public:
    Renderer();
    ~Renderer();

    void update_chart(std::string symbol, double last_price, bool temporary_price, bool to_render);
    void render_gain(std::string symbol, double last_price);
    void render_logo(std::string logo, int size);
    void render_symbol(std::string symbol);
    void render_price(std::string symbol, double last_price);
    void render_entire_symbol(std::string symbol, double price);
    rgb_matrix::RGBMatrix* get_matrix();
private:
    std::unordered_map<std::string, double> closed_market_prices_;
    
    std::unordered_map<std::string, std::deque<double>> past_charts_;

    DataStorage *d = DataStorage::getInstance();

    void fetch_past_chart(std::string symbol, int chart_width);

    rgb_matrix::RGBMatrix* matrix;
    rgb_matrix::RGBMatrix::Options matrix_options;

    Config c = Config(CONFIG_FILE);

    bool logo_rendered = c.get_bool_render_logos();
};

#endif // Renderer_HPP
