#ifndef Renderer_HPP
#define Renderer_HPP

#include "led-matrix.h"
#include <algorithm>
#include <iostream>
#include <deque>
#include <cmath>
#include <unordered_map>
#include "Core/Database/DataStorage.hpp"
#include <cstdlib> // for rand() and srand()
#include <ctime>   // for time()
#include <unistd.h>  // for sleep
#include <signal.h>  // for signal

class Renderer {
public:
    Renderer(int chart_width, int chart_height);
    ~Renderer();

    void render_chart(std::string symbol, double last_price, bool temporary_price);
    void render_percentage(std::string symbol, double last_price);
    void render_logo(std::string logo);
private:
    int chart_width_;
    int chart_height_;
    double day_start_price_ = 0;
    
    std::unordered_map<std::string, std::deque<double>> past_charts_;

    DataStorage *d = DataStorage::getInstance();

    void fetch_past_chart(std::string symbol);

    rgb_matrix::RGBMatrix* matrix;
};



#endif // Renderer_HPP
