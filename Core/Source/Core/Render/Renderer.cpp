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
    rgb_matrix::RuntimeOptions runtimeOpt;

    std::vector<std::string> matrixArgs = {
        "program",
        "--led-cols=" + std::to_string(MATRIX_WIDTH), 
        "--led-slowdown-gpio=" + std::to_string(GPIO_SLOWDOWN), 
        "--led-no-hardware-pulse", 
        "--led-gpio-mapping=" + std::string(GPIO_MAPPING), 
        "--led-rgb-sequence=" + std::string(RGB_SEQUENCE),
    };

    // Convert std::vector<std::string> to argc and argv
    int argc = matrixArgs.size();
    char **argv = new char*[argc];

    for (int i = 0; i < argc; ++i) {
        argv[i] = new char[matrixArgs[i].size() + 1];
        std::strcpy(argv[i], matrixArgs[i].c_str());
    }

    if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv, &matrixOptions_, &runtimeOpt)) {
        std::cout << "Invalid option" << std::endl;
        exit(1);
    }

    for (int i = 0; i < argc; ++i) {
        delete[] argv[i];  // Free each individual char array
    }
    delete[] argv;  // Free the array of pointers

    matrix_ = RGBMatrix::CreateFromOptions(matrixOptions_, runtimeOpt);
    if (matrix_ == NULL){
        std::cerr << "Unable to initialize matrix" << std::endl;
        exit(1);
    }
}

Renderer::~Renderer() {
    std::cout << "Clearing matrix..." << std::endl;
    matrix_->Clear();
    delete matrix_;
    matrix_ = NULL;
    std::cout << "Cleared matrix." << std::endl;
}

void Renderer::updateChart(std::string symbol, double lastPrice, bool temporaryPrice, bool toRender) {
    if (pastCharts_[symbol].empty()) {
        pastCharts_[symbol] = dataStorage_->getPriceHistory(symbol, MATRIX_WIDTH);
    }
    
    int offsetX = logoRendered_ ? config_->getLogoSize() + LOGO_CHART_GAP : 0;
    int offsetY = MATRIX_WIDTH - config_->getChartHeight();

    // print symbol
    std::cout << "symbol: " << symbol << std::endl;

    std::cout << "past chart: " << std::endl;
    for (int i = offsetX; i < MATRIX_WIDTH; i += 1) {
        std::cout << pastCharts_[symbol][i] << " "; 
    }
    std::cout << std::endl;

    // print last price
    std::cout << "last price: " << lastPrice << std::endl; 

    pastCharts_[symbol].push_back(lastPrice);

    // make sure chart_ has <width> number of columns
    if (pastCharts_[symbol].size() > MATRIX_WIDTH) {
        pastCharts_[symbol].pop_front();
    }

    bool hasValue = false;
    for (int i = offsetX; i < MATRIX_WIDTH; i += 1) {
        if (pastCharts_[symbol][i] != MISSING_PRICE) {
            hasValue = true;
            break;
        }
    }

    if (!hasValue) {
        return;
    }

    if (toRender) {
        std::vector<double> renderedChart;
        for (int i = offsetX; i < MATRIX_WIDTH; i += 1) {
            renderedChart.push_back(pastCharts_[symbol][i]);
        }

        // min value in the chart
        double minValue = std::numeric_limits<double>::max(); // Initialize with a large value
        for (double num : renderedChart) if (num != MISSING_PRICE && num < minValue) minValue = num;

        // max value in the chart
        double maxValue = *std::max_element(renderedChart.begin(), renderedChart.end());

        // normalize the chart
        for(int i = 0; i < renderedChart.size(); i += 1){
            if (renderedChart[i] == MISSING_PRICE){
                continue;
            }
            else if (minValue != maxValue){
                renderedChart[i] = ((renderedChart[i] - minValue) / (double)(maxValue - minValue)) * config_->getChartHeight();
            }
            else {
                renderedChart[i] = 0;
            } 
        }

        // clear the gap between the chart and the logo
        if (logoRendered_) {
            for(int y = config_->getChartHeight(); y >= 0; y -= 1){
                matrix_->SetPixel(offsetX-1, matrix_->height() - y - 1, 0, 0, 0);
            } 
        }

        // run through rendered chart and remove everything before first non MISSING_PRICE value
        while(renderedChart[0] == MISSING_PRICE){
            renderedChart.erase(renderedChart.begin());
        }
        
        int renderedChartWidth = renderedChart.size();
        for(int y = config_->getChartHeight(); y >= 0; y -= 1){
            for(int x = 0; x < renderedChartWidth; x += 1){
                matrix_->SetPixel(x + offsetX, matrix_->height() - y - 1, 0, 0, 0);
                // skip missing timepoints
                if (renderedChart[x] == MISSING_PRICE) continue;

                if (y == (int)renderedChart[x]){
                    matrix_->SetPixel(x + offsetX, matrix_->height() - y - 1, chartTopRGB[0], chartTopRGB[1], chartTopRGB[2]);
                }
                else if (y == 0 || y < renderedChart[x]){
                    if ((x > 0 && y > renderedChart[x - 1]) || 
                        (x < renderedChartWidth-1 && y > renderedChart[x + 1])){
                        matrix_->SetPixel(x + offsetX, matrix_->height() - y - 1, chartTopRGB[0], chartTopRGB[1], chartTopRGB[2]);
                    }
                    else {
                        matrix_->SetPixel(x + offsetX, matrix_->height() - y - 1, chartBaseRGB[0], chartBaseRGB[1], chartBaseRGB[2]);
                    }
                }          
            }
        }
    }

    if (temporaryPrice) {
        pastCharts_[symbol].pop_back();
    }
}

void Renderer::renderLogo(std::string logo, int size) {
    if (logo == ""){
        logoRendered_ = false; 
        return;
    }
    if (!fs::exists(fs::path{logo})){
        logoRendered_ = false; 
        std::cout << "Logo not found: " << logo << std::endl;
        return;
    }

    Mat image = imread(logo, IMREAD_COLOR | IMREAD_IGNORE_ORIENTATION);

    const int offsetX = 0, offsetY = matrix_->height() - size;
    // Copy all the pixels to the matrix.
    for (size_t y = 0; y < image.rows; y++) {
        for (size_t x = 0; x < image.cols; x++) {
            int blue = image.at<Vec3b>(y, x)[0];
            int green = image.at<Vec3b>(y, x)[1];
            int red = image.at<Vec3b>(y, x)[2];
            matrix_->SetPixel(x + offsetX, y + offsetY,
                            red,
                            green,
                            blue);
        }
    }

    logoRendered_ = true;
}

void Renderer::renderSymbol(std::string symbol) {
    rgb_matrix::Color fontColor(255, 255, 255);

    std::string bdfFontFile = "fonts/"+ std::to_string(SYMBOL_FONT_WIDTH) + \
                                "x"+std::to_string(SYMBOL_FONT_HEIGHT) +".bdf";
    int xOrig = 2;
    int yOrig = 1;
    int letterSpacing = 0;
    
    //Load font. This needs to be a filename with a bdf bitmap font.
    rgb_matrix::Font font;
    if (!font.LoadFont(bdfFontFile.c_str())) {
        fprintf(stderr, "Couldn't load font '%s'\n", bdfFontFile.c_str());
        exit(1);
    }

    rgb_matrix::DrawText(matrix_, font, xOrig, yOrig + font.baseline(),
                         fontColor, NULL, symbol.c_str(),
                         letterSpacing);

    // draw red vertial line on the left of symbol
    for (int y = yOrig; y < yOrig + font.baseline(); y += 1) {
        matrix_->SetPixel(0, y, 255, 0, 0);
    }
}

void Renderer::renderGain(std::string symbol, double lastPrice) {
    if (closedMarketPrices_[symbol] == ZERO_PRICE) {
        closedMarketPrices_[symbol] = dataStorage_->closedMarketPrice(symbol);
    }

    if (lastPrice == MISSING_PRICE){
        lastPrice = dataStorage_->getLastPrice(symbol);
    }

    double percentage = 0;
    if (closedMarketPrices_[symbol] != ZERO_PRICE && lastPrice != ZERO_PRICE) {
        percentage = ((lastPrice - closedMarketPrices_[symbol]) / closedMarketPrices_[symbol]) * 100;
    }
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(PERCENTAGE_PRECISION) << percentage;
    std::string todaysGain = stream.str() + "%";
    if (percentage > 0) {
        todaysGain = "+" + todaysGain;
    }

    rgb_matrix::Color fontColor;    
    if (percentage > 0.0){
        fontColor = rgb_matrix::Color(0, 255, 0);
    } else if (percentage < 0.0){
        fontColor = rgb_matrix::Color(255, 0, 0);
    } else {
        fontColor = rgb_matrix::Color(255, 255, 255);
    }

    std::string bdfFontFile = "fonts/"+ std::to_string(PERCENTAGE_FONT_WIDTH) + \
                                "x"+std::to_string(PERCENTAGE_FONT_HEIGHT) +".bdf";
    int xOrig = MATRIX_WIDTH-todaysGain.length()*PERCENTAGE_FONT_WIDTH;
    int yOrig = 1 + PERCENTAGE_FONT_HEIGHT + 1;
    int letterSpacing = 0;
    
    //Load font. This needs to be a filename with a bdf bitmap font.
    rgb_matrix::Font font;
    if (!font.LoadFont(bdfFontFile.c_str())) {
        fprintf(stderr, "Couldn't load font '%s'\n", bdfFontFile.c_str());
        exit(1);
    }

    // clear previous text
    for (int y = yOrig; y < yOrig + font.baseline(); y += 1) {
        for (int x = xOrig-PERCENTAGE_FONT_WIDTH*2; \
            x < std::min(static_cast<int>(xOrig + (todaysGain.length()+1)*PERCENTAGE_FONT_WIDTH), MATRIX_WIDTH); \
            x += 1) {
            matrix_->SetPixel(x, y, 0, 0, 0);
        }
    }

    rgb_matrix::DrawText(matrix_, font, xOrig, yOrig + font.baseline(),
                         fontColor, NULL, todaysGain.c_str(),
                         letterSpacing);
}

void Renderer::renderPrice(std::string symbol, double lastPrice) {
    if (lastPrice == MISSING_PRICE){
        lastPrice = dataStorage_->getLastPrice(symbol);
    }

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(5) << lastPrice;
    std::string price = stream.str();
    while (price.length() > 6) {
        price = price.substr(0, price.length()-1);
    }

    // if last price symbol is . then remove it
    if (price[price.length()-1] == '.') {
        price = price.substr(0, price.length()-1);
    }

    price = "$" + price;

    rgb_matrix::Color fontColor(255, 255, 255);
    
    std::string bdfFontFile = "fonts/"+ std::to_string(PRICE_FONT_WIDTH) + \
                                "x"+std::to_string(PRICE_FONT_HEIGHT) +".bdf";
    int xOrig = logoRendered_ ? MATRIX_WIDTH-price.length()*PRICE_FONT_WIDTH : 2;
    int yOrig = logoRendered_ ? 1 : 1 + PRICE_FONT_HEIGHT + 1;
    int letterSpacing = 0;

    //Load font. This needs to be a filename with a bdf bitmap font.
    rgb_matrix::Font font;
    if (!font.LoadFont(bdfFontFile.c_str())) {
        fprintf(stderr, "Couldn't load font '%s'\n", bdfFontFile.c_str());
        exit(1);
    }

    // clear previous text
    for (int y = yOrig; y < yOrig + font.baseline()+1; y += 1) {
        for (int x = xOrig-PRICE_FONT_WIDTH; \
            x < std::min(static_cast<int>(xOrig + (price.length() + 1) * PRICE_FONT_WIDTH), MATRIX_WIDTH); \
            x += 1){
            matrix_->SetPixel(x, y, 0, 0, 0);
        }
    }

    rgb_matrix::DrawText(matrix_, font, xOrig, yOrig + font.baseline(),
                         fontColor, NULL, price.c_str(),
                         letterSpacing);
}

void Renderer::renderEntireSymbol(int currentSymbolIndex, double price) {
    std::string symbolName = config_->getSubsList()[currentSymbolIndex];
    std::string apiSymbolName = config_->getApiSubsList()[currentSymbolIndex];
    std::string logoSymbolName = config_->getLogoSubsList()[currentSymbolIndex];

    matrix_->Fill(0, 0, 0);
    matrix_->Clear();
    renderLogo(LOGO_DIR+"/"+logoSymbolName+LOGO_EXT, config_->getLogoSize());
    renderSymbol(symbolName);
    renderPrice(apiSymbolName, price);
    renderGain(apiSymbolName, price);
    updateChart(apiSymbolName, price, true, true);
}

void Renderer::clearPastCharts(){
    // clear past chart map for the symbol
    pastCharts_ = std::unordered_map<std::string, std::deque<double>>();
}

rgb_matrix::RGBMatrix* Renderer::getMatrix() {
    return matrix_;
}
