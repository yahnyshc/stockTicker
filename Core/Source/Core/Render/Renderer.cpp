#include <opencv2/highgui.hpp>
#include <filesystem>
#include <algorithm> // for std::min
#include "Renderer.hpp"

using rgb_matrix::Canvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::FrameCanvas;
using namespace cv;
namespace fs = std::filesystem;

Renderer::Renderer() {
    // Initialize the RGB matrix with
    rgb_matrix::RuntimeOptions runtime_opt;

    std::vector<std::string> matrix_args = {
        "program",
        "--led-cols=64", 
        "--led-slowdown-gpio=4", 
        "--led-no-hardware-pulse", 
        "--led-gpio-mapping=adafruit-hat", 
        "--led-rgb-sequence=RBG"
    };

    // Convert std::vector<std::string> to argc and argv
    int argc = matrix_args.size();
    char **argv = new char*[argc];

    for (int i = 0; i < argc; ++i) {
        argv[i] = new char[matrix_args[i].size() + 1];
        std::strcpy(argv[i], matrix_args[i].c_str());
    }

    if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv, &matrix_options, &runtime_opt)) {
        std::cout << "Invalid option" << std::endl;
        exit(1);
    }

    for (int i = 0; i < argc; ++i) {
        delete[] argv[i];  // Free each individual char array
    }
    delete[] argv;  // Free the array of pointers

    matrix = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
    if (matrix == NULL){
        std::cerr << "Unable to initialize matrix" << std::endl;
        exit(1);
    }
}

Renderer::~Renderer() {
    std::cout << "Clearing matrix..." << std::endl;
    matrix->Clear();
    delete matrix;
    matrix = NULL;
    std::cout << "Cleared matrix." << std::endl;
}

void Renderer::fetch_past_chart(std::string symbol, int chart_width) {
    past_charts_[symbol] = d->get_price_history(symbol, chart_width - 1);
}

void Renderer::update_chart(std::string symbol, double last_price, bool temporary_price, bool to_render) {
    int chart_width = 64;
    int offset_x = logo_rendered ? c.get_logo_size() + 1 : 0;
    int offset_y = 64 - c.get_chart_height();

    if (past_charts_[symbol].empty()) {
        fetch_past_chart(symbol, 64);
    }
    past_charts_[symbol].push_back(last_price);

    std::cout << "past chart: " << std::endl;
    // print past chart
    for (int i = offset_x; i < chart_width; i += 1) {
        std::cout << past_charts_[symbol][i] << "\t";
    }
    std::cout << std::endl;

    // make sure chart_ has <width> number of columns
    if (past_charts_[symbol].size() > chart_width) {
        past_charts_[symbol].pop_front();
    }

    if (to_render) {
        std::vector<double> rendered_chart;
        for (int i = offset_x; i < chart_width; i += 1) {
            rendered_chart.push_back(past_charts_[symbol][i]);
        }

        // min value in the chart
        double min_value = std::numeric_limits<double>::max(); // Initialize with a large value
        for (double num : rendered_chart) if (num != -1 && num < min_value) min_value = num;

        // max value in the chart
        double max_value = *std::max_element(rendered_chart.begin(), rendered_chart.end());

        std::cout << "Min value: " << min_value << " Max value: " << max_value << std::endl;

        // normalize the chart
        for(int i = 0; i < rendered_chart.size(); i += 1){
            if (rendered_chart[i] < 0){
                continue;
            }
            else if (min_value != max_value){
                rendered_chart[i] = ((rendered_chart[i] - min_value) / (double)(max_value - min_value)) * c.get_chart_height();
            }
            else {
                rendered_chart[i] = 0;
            } 
        }

        // print the chart
        for(int i = c.get_chart_height(); i >= 0; i -= 1){
            for(int j = 0; j < rendered_chart.size(); j += 1){
                std::cout << (i == 0 || i <= rendered_chart[j] ? "*" : " ");
            }
            std::cout << std::endl;
        }

        // render chart to the bottom right corner
        std::cout << "chart height: " << c.get_chart_height() << std::endl;

        // clear the gap between the chart and the logo
        if (logo_rendered) {
            for(int y = c.get_chart_height(); y >= 0; y -= 1){
                matrix->SetPixel(offset_x-1, matrix->height() - y - 1, 0, 0, 0);
            } 
        }
        
        int rendered_chart_width = rendered_chart.size();
        for(int y = c.get_chart_height(); y >= 0; y -= 1){
            for(int x = 0; x < rendered_chart_width; x += 1){
                matrix->SetPixel(x + offset_x, matrix->height() - y - 1, 0, 0, 0);
                // skip missing timepoints
                if (rendered_chart[x] == -1) continue;

                if (y == (int)rendered_chart[x]){
                    matrix->SetPixel(x + offset_x, matrix->height() - y - 1, 0, 255, 77);
                }
                else if (y == 0 || y < rendered_chart[x]){
                    if ((x > 0 && y > rendered_chart[x - 1]) || 
                        (x < rendered_chart_width-1 && y > rendered_chart[x + 1])){
                        matrix->SetPixel(x + offset_x, matrix->height() - y - 1, 0, 255, 77);
                    }
                    else {
                        matrix->SetPixel(x + offset_x, matrix->height() - y - 1, 0, 150, 0);
                    }
                }          
            }
        }
    }

    if (temporary_price) {
        past_charts_[symbol].pop_back();
    }
}

void Renderer::render_logo(std::string logo, int size) {
    if ( ! fs::exists(fs::path{logo}) ){
        logo_rendered = false; 
        std::cout << "Logo not found: " << logo << std::endl;
        return;
    }

    Mat image = imread(logo, IMREAD_COLOR | IMREAD_IGNORE_ORIENTATION);

    const int offset_x = 0, offset_y = matrix->height() - size;
    // Copy all the pixels to the matrix.
    for (size_t y = 0; y < image.rows; y++) {
        for (size_t x = 0; x < image.cols; x++) {
            int blue = image.at<Vec3b>(y, x)[0];
            int green = image.at<Vec3b>(y, x)[1];
            int red = image.at<Vec3b>(y, x)[2];
            // std::cout << "x: " << x << " y: " << y << " red: " << red << " green: " << green << " blue: " << blue << "\n";
            matrix->SetPixel(x + offset_x, y + offset_y,
                            red,
                            green,
                            blue);
        }
    }

    logo_rendered = true;
}

void Renderer::render_symbol(std::string symbol) {
    rgb_matrix::Color font_color(255, 255, 255);

    const char *bdf_font_file = "fonts/5x7.bdf";
    int x_orig = 2;
    int y_orig = 1;
    int letter_spacing = 0;
    
    //Load font. This needs to be a filename with a bdf bitmap font.
    rgb_matrix::Font font;
    if (!font.LoadFont(bdf_font_file)) {
        fprintf(stderr, "Couldn't load font '%s'\n", bdf_font_file);
        exit(1);
    }

    rgb_matrix::DrawText(matrix, font, x_orig, y_orig + font.baseline(),
                         font_color, NULL, symbol.c_str(),
                         letter_spacing);

    // draw red vertial line on the left of symbol
    for (int y = y_orig; y < y_orig + font.baseline(); y += 1) {
        matrix->SetPixel(0, y, 255, 0, 0);
    }
}

void Renderer::render_gain(std::string symbol, double last_price) {
    if (closed_market_prices_[symbol] == 0.0) {
        closed_market_prices_[symbol] = d->closed_market_price(symbol);
    }

    double percentage = 0;
    if (closed_market_prices_[symbol] != 0.0 && last_price != 0.0) {
        percentage = ((last_price - closed_market_prices_[symbol]) / closed_market_prices_[symbol]) * 100;
    }
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << percentage;
    std::string todays_gain = stream.str() + "%";
    if (percentage > 0) {
        todays_gain = "+" + todays_gain;
    }

    rgb_matrix::Color font_color;    
    if (percentage > 0.0){
        font_color = rgb_matrix::Color(0, 255, 0);
    } else if (percentage < 0.0){
        font_color = rgb_matrix::Color(255, 0, 0);
    } else {
        font_color = rgb_matrix::Color(255, 255, 255);
    }

    const char *bdf_font_file = "fonts/4x6.bdf";
    int x_orig = 64-todays_gain.length()*4;
    int y_orig = 1 + 6 + 1;
    int letter_spacing = 0;
    
    //Load font. This needs to be a filename with a bdf bitmap font.
    rgb_matrix::Font font;
    if (!font.LoadFont(bdf_font_file)) {
        fprintf(stderr, "Couldn't load font '%s'\n", bdf_font_file);
        exit(1);
    }

    // clear previous text
    for (int y = y_orig; y < y_orig + font.baseline(); y += 1) {
        for (int x = x_orig-4*2; x < std::min(static_cast<int>(x_orig + (todays_gain.length()+1)*4), 64); x += 1) {
            matrix->SetPixel(x, y, 0, 0, 0);
        }
    }

    rgb_matrix::DrawText(matrix, font, x_orig, y_orig + font.baseline(),
                         font_color, NULL, todays_gain.c_str(),
                         letter_spacing);
}

void Renderer::render_price(double last_price) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(5) << last_price;
    std::string price = stream.str();
    while (price.length() > 6) {
        price = price.substr(0, price.length()-1);
    }

    // if last price symbol is . then remove it
    if (price[price.length()-1] == '.') {
        price = price.substr(0, price.length()-1);
    }

    price = "$" + price;
    std::cout << "Last price: " << price << std::endl;

    rgb_matrix::Color font_color(255, 255, 255);
    
    const char *bdf_font_file = "fonts/4x6.bdf";
    int x_orig = logo_rendered ? 64-price.length()*4 : 2;
    int y_orig = logo_rendered ? 1 : 1 + 6 + 1;
    int letter_spacing = 0;

    //Load font. This needs to be a filename with a bdf bitmap font.
    rgb_matrix::Font font;
    if (!font.LoadFont(bdf_font_file)) {
        fprintf(stderr, "Couldn't load font '%s'\n", bdf_font_file);
        exit(1);
    }

    // clear previous text
    for (int y = y_orig; y < y_orig + font.baseline(); y += 1) {
        for (int x = x_orig-4; x < std::min(static_cast<int>(x_orig + (price.length() + 1) * 4), 64); x += 1){
            matrix->SetPixel(x, y, 0, 0, 0);
        }
    }

    rgb_matrix::DrawText(matrix, font, x_orig, y_orig + font.baseline(),
                         font_color, NULL, price.c_str(),
                         letter_spacing);
}

rgb_matrix::RGBMatrix* Renderer::get_matrix() {
    return matrix;
}