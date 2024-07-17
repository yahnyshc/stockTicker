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
        "--led-cols=" + std::to_string(MATRIX_WIDTH), 
        "--led-slowdown-gpio=" + std::to_string(GPIO_SLOWDOWN), 
        "--led-no-hardware-pulse", 
        "--led-gpio-mapping=" + std::string(GPIO_MAPPING), 
        "--led-rgb-sequence=" + std::string(RGB_SEQUENCE),
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

    // fetch past charts
    for (const auto& symbol : c.get_symbols()) {
        fetch_past_chart(symbol, MATRIX_WIDTH);
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
    past_charts_[symbol] = d->get_price_history(symbol, chart_width);
}

void Renderer::update_chart(std::string symbol, double last_price, bool temporary_price, bool to_render) {
    int offset_x = logo_rendered ? c.get_logo_size() + LOGO_CHART_GAP : 0;
    int offset_y = MATRIX_WIDTH - c.get_chart_height();

    // print symbol
    std::cout << "symbol: " << symbol << std::endl;

    std::cout << "past chart: " << std::endl;
    for (int i = offset_x; i < MATRIX_WIDTH; i += 1) {
        std::cout << past_charts_[symbol][i] << " "; 
    }
    std::cout << std::endl;

    // print last price
    std::cout << "last price: " << last_price << std::endl; 

    past_charts_[symbol].push_back(last_price);

    // make sure chart_ has <width> number of columns
    if (past_charts_[symbol].size() > MATRIX_WIDTH) {
        past_charts_[symbol].pop_front();
    }

    bool has_value = false;
    for (int i = offset_x; i < MATRIX_WIDTH; i += 1) {
        if (past_charts_[symbol][i] != MISSING_PRICE) {
            has_value = true;
            break;
        }
    }

    if (!has_value) {
        return;
    }

    if (to_render) {
        std::vector<double> rendered_chart;
        for (int i = offset_x; i < MATRIX_WIDTH; i += 1) {
            rendered_chart.push_back(past_charts_[symbol][i]);
        }

        // min value in the chart
        double min_value = std::numeric_limits<double>::max(); // Initialize with a large value
        for (double num : rendered_chart) if (num != MISSING_PRICE && num < min_value) min_value = num;

        // max value in the chart
        double max_value = *std::max_element(rendered_chart.begin(), rendered_chart.end());

        // normalize the chart
        for(int i = 0; i < rendered_chart.size(); i += 1){
            if (rendered_chart[i] == MISSING_PRICE){
                continue;
            }
            else if (min_value != max_value){
                rendered_chart[i] = ((rendered_chart[i] - min_value) / (double)(max_value - min_value)) * c.get_chart_height();
            }
            else {
                rendered_chart[i] = 0;
            } 
        }

        // clear the gap between the chart and the logo
        if (logo_rendered) {
            for(int y = c.get_chart_height(); y >= 0; y -= 1){
                matrix->SetPixel(offset_x-1, matrix->height() - y - 1, 0, 0, 0);
            } 
        }

        // run through rendered chart and remove everything before first non MISSING_PRICE value
        while(rendered_chart[0] == MISSING_PRICE){
            rendered_chart.erase(rendered_chart.begin());
        }
        
        int rendered_chart_width = rendered_chart.size();
        for(int y = c.get_chart_height(); y >= 0; y -= 1){
            for(int x = 0; x < rendered_chart_width; x += 1){
                matrix->SetPixel(x + offset_x, matrix->height() - y - 1, 0, 0, 0);
                // skip missing timepoints
                if (rendered_chart[x] == MISSING_PRICE) continue;

                if (y == (int)rendered_chart[x]){
                    matrix->SetPixel(x + offset_x, matrix->height() - y - 1, chart_top_RGB[0], chart_top_RGB[1], chart_top_RGB[2]);
                }
                else if (y == 0 || y < rendered_chart[x]){
                    if ((x > 0 && y > rendered_chart[x - 1]) || 
                        (x < rendered_chart_width-1 && y > rendered_chart[x + 1])){
                        matrix->SetPixel(x + offset_x, matrix->height() - y - 1, chart_top_RGB[0], chart_top_RGB[1], chart_top_RGB[2]);
                    }
                    else {
                        matrix->SetPixel(x + offset_x, matrix->height() - y - 1, chart_base_RGB[0], chart_base_RGB[1], chart_base_RGB[2]);
                    }
                }          
            }
        }
    }

    if ( temporary_price ) {
        past_charts_[symbol].pop_back();
    }
}

void Renderer::render_logo(std::string logo, int size) {
    if ( ! c.get_bool_render_logos() ){
        logo_rendered = false; 
        return;
    }
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
    if (closed_market_prices_[symbol] == ZERO_PRICE) {
        closed_market_prices_[symbol] = d->closed_market_price(symbol);
    }

    if (last_price == MISSING_PRICE){
        last_price = d->get_last_price(symbol);
    }

    double percentage = 0;
    if (closed_market_prices_[symbol] != ZERO_PRICE && last_price != ZERO_PRICE) {
        percentage = ((last_price - closed_market_prices_[symbol]) / closed_market_prices_[symbol]) * 100;
    }
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(PERCENTAGE_PRECISION) << percentage;
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

    const char *bdf_font_file = "fonts/"+ std::to_string(PERCENTAGE_FONT_WIDTH) + \
                                "x"+std::to_string(PERCENTAGE_FONT_HEIGHT) +".bdf";
    int x_orig = MATRIX_WIDTH-todays_gain.length()*PERCENTAGE_FONT_WIDTH;
    int y_orig = 1 + PERCENTAGE_FONT_HEIGHT + 1;
    int letter_spacing = 0;
    
    //Load font. This needs to be a filename with a bdf bitmap font.
    rgb_matrix::Font font;
    if (!font.LoadFont(bdf_font_file)) {
        fprintf(stderr, "Couldn't load font '%s'\n", bdf_font_file);
        exit(1);
    }

    // clear previous text
    for (int y = y_orig; y < y_orig + font.baseline(); y += 1) {
        for (int x = x_orig-PERCENTAGE_FONT_WIDTH*2; \
            x < std::min(static_cast<int>(x_orig + (todays_gain.length()+1)*PERCENTAGE_FONT_WIDTH), MATRIX_WIDTH); \
            x += 1) {
            matrix->SetPixel(x, y, 0, 0, 0);
        }
    }

    rgb_matrix::DrawText(matrix, font, x_orig, y_orig + font.baseline(),
                         font_color, NULL, todays_gain.c_str(),
                         letter_spacing);
}

void Renderer::render_price(std::string symbol, double last_price) {
    if (last_price == MISSING_PRICE){
        last_price = d->get_last_price(symbol);
    }

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

    rgb_matrix::Color font_color(255, 255, 255);
    
    const char *bdf_font_file = "fonts/"+ std::to_string(PRICE_FONT_WIDTH) + \
                                "x"+std::to_string(PRICE_FONT_HEIGHT) +".bdf";
    int x_orig = logo_rendered ? MATRIX_WIDTH-price.length()*PRICE_FONT_WIDTH : 2;
    int y_orig = logo_rendered ? 1 : 1 + PRICE_FONT_HEIGHT + 1;
    int letter_spacing = 0;

    //Load font. This needs to be a filename with a bdf bitmap font.
    rgb_matrix::Font font;
    if (!font.LoadFont(bdf_font_file)) {
        fprintf(stderr, "Couldn't load font '%s'\n", bdf_font_file);
        exit(1);
    }

    // clear previous text
    for (int y = y_orig; y < y_orig + font.baseline()+1; y += 1) {
        for (int x = x_orig-PRICE_FONT_WIDTH; \
            x < std::min(static_cast<int>(x_orig + (price.length() + 1) * PRICE_FONT_WIDTH), MATRIX_WIDTH); \
            x += 1){
            matrix->SetPixel(x, y, 0, 0, 0);
        }
    }

    rgb_matrix::DrawText(matrix, font, x_orig, y_orig + font.baseline(),
                         font_color, NULL, price.c_str(),
                         letter_spacing);
}

void Renderer::render_entire_symbol(std::string symbol, double price) {
    matrix->Fill(0, 0, 0);
    render_logo(LOGO_DIR+"/"+symbol_to_logo(symbol)+LOGO_EXT, c.get_logo_size());
    render_symbol(symbol_to_logo(symbol));
    render_price(symbol, price);
    render_gain(symbol, price);
    update_chart(symbol, price, true, true);
}

rgb_matrix::RGBMatrix* Renderer::get_matrix() {
    return matrix;
}