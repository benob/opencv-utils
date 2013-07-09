#include <iostream>

#include <opencv2/opencv.hpp>
#include "bow.h"

int main(int argc, char** argv) {
    //amu::BOWFeatures features("bow.vocabulary.1000");
    amu::BOWFeatures features("bow-vocabulary.keyframes-png-v6-train.1000");
    cv::Mat original, image, descriptors, previous;
    std::string filename;

    int count = 0;
    while(std::getline(std::cin, filename)) {
        if(count % 10 == 0) {
        original = cv::imread(filename);
        cv::resize(original, image, cv::Size(1024 / 2, 576 / 2));
        //features.Extract(image, descriptors);
        cv::imshow("frame", image);
        cv::waitKey(10);
        //descriptors.copyTo(previous);
        }
        count += 1;
    }
}
