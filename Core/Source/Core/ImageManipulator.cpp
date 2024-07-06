#include <opencv2/opencv.hpp>
#include <iostream>
#include "ImageManipulator.hpp"

using namespace cv;

ImageManipulator::ImageManipulator(std::string filename){
    filename_ = filename;
}

void ImageManipulator::reduce(int width, int height){
    Mat image = imread(filename_);    
    Mat scaled_img;
    // resize
    resize(image, scaled_img, Size(width, height), INTER_LINEAR);

    // Save the resized image by overwriting the same file
    bool success = imwrite( filename_, scaled_img);
    
    if ( ! success ) {
        std::cerr << "Error: Failed to save the resized image." << std::endl;
    }
}