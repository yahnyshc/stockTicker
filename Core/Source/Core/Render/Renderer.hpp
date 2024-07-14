#ifndef Renderer_HPP
#define Renderer_HPP

#include "led-matrix.h"
#include "graphics.h"
#include <algorithm>
#include <iostream>
#include <deque>
#include <cmath>
#include <unordered_map>
#include "Core/Database/DataStorage.hpp"
#include "Core/Api/Config.hpp"
#include <cstdlib> // for rand() and srand()
#include <ctime>   // for time()
#include <unistd.h>  // for sleep

class Renderer {
public:
    Renderer();
    ~Renderer();

    void render_chart(std::string symbol, double last_price, bool temporary_price);
    void render_percentage(std::string symbol, double last_price);
    void render_logo(std::string logo, int size);
    void render_symbol(std::string symbol);
private:
    double day_start_price_ = 0;
    
    std::unordered_map<std::string, std::deque<double>> past_charts_;

    DataStorage *d = DataStorage::getInstance();

    void fetch_past_chart(std::string symbol);

    rgb_matrix::RGBMatrix* matrix;
    rgb_matrix::RGBMatrix::Options matrix_options;

    Config c = Config("ws.cfg");

};



#endif // Renderer_HPP
