#ifndef Renderer_HPP
#define Renderer_HPP

#include <algorithm>
#include <iostream>
#include <deque>
#include <cmath>
#include <unordered_map>
#include "Core/Database/DataStorage.hpp"
#include <cstdlib> // for rand() and srand()
#include <ctime>   // for time()

class Renderer {
public:
    Renderer(int chart_width, int chart_height);

    void render_chart(std::string symbol, double last_price, bool temporary_price);
    void render_percentage(std::string symbol, double last_price);

private:
    int chart_width_;
    int chart_height_;
    double day_start_price_ = 0;
    
    std::unordered_map<std::string, std::deque<double>> past_charts_;

    DataStorage *d = DataStorage::getInstance();

    void fetch_past_chart(std::string symbol);
};



#endif // Renderer_HPP