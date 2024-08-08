#ifndef RENDERER_HPP
#define RENDERER_HPP

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
#include <unistd.h> // for sleep
#include "Core/Database/DataStorage.hpp"
#include "Core/Config.hpp"
#include "Core/GlobalParams.hpp"

constexpr int MATRIX_WIDTH = 64;
constexpr int GPIO_SLOWDOWN = 4;
const std::string GPIO_MAPPING = "adafruit-hat";
const std::string RGB_SEQUENCE = "RBG";

constexpr int LOGO_CHART_GAP = 1;
constexpr int SYMBOL_LEFT_SPACING = 2;
constexpr int SYMBOL_TOP_SPACING = 1;

constexpr int PERCENTAGE_PRECISION = 2;

constexpr int PERCENTAGE_FONT_WIDTH = 4;
constexpr int PERCENTAGE_FONT_HEIGHT = 6;
constexpr int PRICE_FONT_WIDTH = 4;
constexpr int PRICE_FONT_HEIGHT = 6;
constexpr int SYMBOL_FONT_WIDTH = 5;
constexpr int SYMBOL_FONT_HEIGHT = 7;

const int chartBaseRGB[3] = {0, 140, 0};
const int chartTopRGB[3] = {0, 255, 90};   

class Renderer {
public:
    Renderer();
    ~Renderer();

    void updateChart(std::string symbol, double lastPrice, bool savePrice, bool toRender);
    void renderGain(std::string symbol, double lastPrice);
    void renderLogo(std::string logo, int size);
    void renderSymbol(std::string symbol);
    void renderPrice(std::string symbol, double lastPrice);
    void renderEntireSymbol(int currentSymbolIndex, double price);
    void clearPastCharts();

    rgb_matrix::RGBMatrix* getMatrix();

private:
    std::unordered_map<std::string, double> closedMarketPrices_;
    std::unordered_map<std::string, std::deque<double>> pastCharts_;

    DataStorage* dataStorage_ = DataStorage::getInstance();

    rgb_matrix::RGBMatrix* matrix_;
    rgb_matrix::RGBMatrix::Options matrixOptions_;

    Config *config_ = Config::getInstance(CONFIG_FILE);

    bool logoRendered_ = false;
};

#endif // RENDERER_HPP
