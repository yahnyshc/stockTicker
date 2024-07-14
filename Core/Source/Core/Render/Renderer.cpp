#include <opencv2/highgui.hpp>
#include "Renderer.hpp"

using rgb_matrix::Canvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::FrameCanvas;
using namespace cv;

static Mat load_image(std::string filename) {
    Mat img = imread(filename, IMREAD_COLOR | IMREAD_IGNORE_ORIENTATION); //loading an image//
    return img;
}

Renderer::Renderer() {
    // Initialize the RGB matrix with
    RGBMatrix::Options matrix_options;
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

    matrix = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
    if (matrix == NULL){
        std::cerr << "Unable to initialize matrix" << std::endl;
        exit(1);
    }
}

Renderer::~Renderer() {
    matrix->Clear();
    delete matrix;
}

void Renderer::fetch_past_chart(std::string symbol) {
    past_charts_[symbol] = d->get_price_history(symbol, c.get_chart_width() - 1);
}

void Renderer::render_chart(std::string symbol, double last_price, bool temporary_price) {
    int offset_x = c.get_logo_size() + 1, offset_y = 64 - c.get_chart_height();

    if (past_charts_[symbol].empty() && d->seconds_since_last_update(symbol) < 60) {
        fetch_past_chart(symbol);
    } else{
        past_charts_[symbol].push_back(last_price);
    }

    std::cout << "past chart: " << std::endl;
    // print past chart
    for (int i = 0; i < c.get_chart_width(); i += 1) {
        std::cout << past_charts_[symbol][i] << "\t";
    }
    std::cout << std::endl;

    std::cout << "past chart length: " << past_charts_[symbol].size() << std::endl;
    // make sure chart_ has <width> number of columns
    if (past_charts_[symbol].size() > c.get_chart_width()) {
        past_charts_[symbol].pop_front();
    }

    std::vector<double> renedered_chart;
    for (int i = 0; i < c.get_chart_width(); i += 1) {
        renedered_chart.push_back(past_charts_[symbol][i]);
    }

    // min value in the chart
    double min_value = std::numeric_limits<double>::max(); // Initialize with a large value
    for (double num : renedered_chart) if (num != 0 && num < min_value) min_value = num;

    // max value in the chart
    double max_value = *std::max_element(renedered_chart.begin(), renedered_chart.end());

    std::cout << "Min value: " << min_value << " Max value: " << max_value << std::endl;

    // normalize the chart
    for(int i = 0; i < c.get_chart_width(); i += 1){
        if (min_value != max_value){
            renedered_chart[i] = ((renedered_chart[i] - min_value) / (double)(max_value - min_value)) * c.get_chart_height();
        }
        else{
            renedered_chart[i] = 0;
        } 
    }

    // print the chart
    for(int i = c.get_chart_height(); i >= 0; i -= 1){
        for(int j = 0; j < c.get_chart_width(); j += 1){
            std::cout << (i == 0 || i <= renedered_chart[j] ? "*" : " ");
        }
        std::cout << std::endl;
    }

    if (temporary_price) {
        past_charts_[symbol].pop_back();
    }
}

void Renderer::render_percentage(std::string symbol, double last_price) {
    if (day_start_price_ == 0.0) {
        day_start_price_ = d->day_start_price(symbol);
    }

    double percentage = ((last_price - day_start_price_) / day_start_price_) * 100;
    std::cout << "Today's Percentage: " << percentage << "%" << std::endl;
}

void Renderer::render_logo(std::string logo, int size) {
    Mat image = load_image(logo);

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
}
