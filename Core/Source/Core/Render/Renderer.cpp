#include <opencv2/highgui.hpp>
#include "Renderer.hpp"

using rgb_matrix::Canvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::FrameCanvas;
using namespace cv;

// Make sure we can exit gracefully when Ctrl-C is pressed.
volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

static Mat LoadImageAndScaleImage(std::string filename) {
    Mat img = imread(filename, IMREAD_COLOR | IMREAD_IGNORE_ORIENTATION); //loading an image//
    return img;
}

int usage(const char *progname) {
  fprintf(stderr, "Usage: %s [led-matrix-options] <image-filename>\n",
          progname);
  rgb_matrix::PrintMatrixFlags(stderr);
  return 1;
}

Renderer::Renderer(int chart_width, int chart_height) {
    chart_width_ = chart_width;
    chart_height_ = chart_height;

    // Initialize the RGB matrix with
    RGBMatrix::Options matrix_options;
    rgb_matrix::RuntimeOptions runtime_opt;

    std::vector<std::string> args = {
        "program",
        "--led-cols=64", 
        "--led-slowdown-gpio=4", 
        "--led-no-hardware-pulse", 
        "--led-gpio-mapping=adafruit-hat", 
        "--led-rgb-sequence=RBG"
    };

    // Convert std::vector<std::string> to argc and argv
    int argc = args.size();
    char **argv = new char*[argc];

    for (int i = 0; i < argc; ++i) {
        argv[i] = new char[args[i].size() + 1];
        std::strcpy(argv[i], args[i].c_str());
    }

    if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv,
                                           &matrix_options, &runtime_opt)) {
        std::cout << "Invalid option" << std::endl;
        exit(1);
    }
    
    // signal(SIGTERM, InterruptHandler);
    // signal(SIGINT, InterruptHandler);

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
    past_charts_[symbol] = d->get_price_history(symbol, chart_width_ - 1);
}

void Renderer::render_chart(std::string symbol, double last_price, bool temporary_price) {
    if (past_charts_[symbol].empty() && d->seconds_since_last_update(symbol) < 60) {
        fetch_past_chart(symbol);
    }
    else{
        past_charts_[symbol].push_back(last_price);
    }

    std::cout << "past chart: " << std::endl;
    // print past chart
    for (int i = 0; i < chart_width_; i += 1) {
        std::cout << past_charts_[symbol][i] << "\t";
    }
    std::cout << std::endl;

    std::cout << "past chart length: " << past_charts_[symbol].size() << std::endl;
    // make sure chart_ has <width> number of columns
    if (past_charts_[symbol].size() > chart_width_) {
        past_charts_[symbol].pop_front();
    }

    std::vector<double> renedered_chart;
    for (int i = 0; i < chart_width_; i += 1) {
        renedered_chart.push_back(past_charts_[symbol][i]);
    }

    // min value in the chart
    double min_value = std::numeric_limits<double>::max(); // Initialize with a large value
    for (double num : renedered_chart) if (num != 0 && num < min_value) min_value = num;

    // max value in the chart
    double max_value = *std::max_element(renedered_chart.begin(), renedered_chart.end());

    std::cout << "Min value: " << min_value << " Max value: " << max_value << std::endl;

    // normalize the chart
    for(int i = 0; i < chart_width_; i += 1){
        if (min_value != max_value){
            renedered_chart[i] = ((renedered_chart[i] - min_value) / (double)(max_value - min_value)) * chart_height_;
        }
        else{
            renedered_chart[i] = 0;
        } 
    }

    // print the chart
    for(int i = chart_height_; i >= 0; i -= 1){
        for(int j = 0; j < chart_width_; j += 1){
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

void CopyImageToCanvas(const Mat image, Canvas *canvas) {
    const int offset_x = 0, offset_y = canvas->height() - 20;
    // Copy all the pixels to the canvas.
    for (size_t y = 0; y < image.rows; ++y) {
        for (size_t x = 0; x < image.cols; ++x) {
            int blue = image.at<Vec3b>(y, x)[0];
            int green = image.at<Vec3b>(y, x)[1];
            int red = image.at<Vec3b>(y, x)[2];
            // std::cout << "x: " << x << " y: " << y << " red: " << red << " green: " << green << " blue: " << blue << "\n";
            canvas->SetPixel(x + offset_x, y + offset_y,
                            red,
                            green,
                            blue);
        }
    }
}


void Renderer::render_logo(std::string logo) {
    Mat image = LoadImageAndScaleImage(logo);

    CopyImageToCanvas(image, matrix);
    // while (!interrupt_received) sleep(1000);  // Until Ctrl-C is pressed
}
