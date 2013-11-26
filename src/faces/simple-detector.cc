#include <iostream>
#include <opencv2/opencv.hpp>

double skin(cv::Mat &image) {
    cv::Mat hsv;
    cv::cvtColor(image, hsv, CV_BGR2HSV);
    cv::Mat hue(image.size(), CV_8UC1), saturation(image.size(), CV_8UC1), value(image.size(), CV_8UC1);
    std::vector<cv::Mat> channels;
    channels.push_back(hue);
    channels.push_back(saturation);
    channels.push_back(value);
    cv::split(hsv, channels);
    cv::threshold(hue, hue, 18, 255, CV_THRESH_BINARY_INV);
    cv::threshold(saturation, saturation, 50, 255, CV_THRESH_BINARY);
    cv::threshold(value, value, 80, 255, CV_THRESH_BINARY);
    cv::Mat output;
    cv::bitwise_and(hue, saturation, output);
    cv::bitwise_and(output, value, output);
    cv::Scalar sums = cv::sum(output);
    double result = sums[0] / (output.rows * output.cols * 255);
    return result;
}

int main(int argc, char** argv) {
    if(argc < 2 || argc > 3) {
        std::cerr << "USAGE: " << argv[0] << " <haar-model> <resize>\n";
        return 1;
    }
    std::string model = argv[1];
    bool resize = (argc == 3);
    cv::CascadeClassifier detector(model);
    if(detector.empty()) {
        std::cerr << "ERROR: could not find model \"" << model << "\"\n";
        return 1;
    }
    std::string line;
    while(std::getline(std::cin, line)) {
        cv::Mat image = cv::imread(line);
        if(image.empty()) {
            std::cerr << "ERROR: could not load image \"" << line << "\"\n";
        } else {
            if(resize) cv::resize(image, image, cv::Size(1024, 576));
            std::vector<cv::Rect> detections;
            detector.detectMultiScale(image, detections);
            std::vector<cv::Rect>::iterator detection = detections.begin();
            while(detection != detections.end()) {
                cv::Mat face(image, *detection);
                if(skin(face) < 0.3) {
                    detection = detections.erase(detection);
                } else {
                    detection ++;
                }
            }
            std::cout << line << " " << detections.size();
            for(size_t i = 0; i < detections.size(); i++) {
                std::cout << " " << detections[i].x << " " << detections[i].y << " " << detections[i].width << " " << detections[i].height;
            } 
            std::cout << "\n";
        }
    }
}
