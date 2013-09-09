#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

int main(int argc, char** argv) {
    if(argc != 1) {
        std::cerr << "usage: cat image-list.txt | %s\n";
        return 1;
    }

    cv::HOGDescriptor hog(cv::Size(128, 64), cv::Size(16,16), cv::Size(8,8), cv::Size(8,8), 9);

    std::string line;
    while(std::getline(std::cin, line)) {
        cv::Mat image = cv::imread(line, 0);
        if(image.rows == 0) {
            std::cerr << "ERROR: reading \"" << line << "\"\n";
        } else {
            cv::resize(image, image, cv::Size(128, 64));

            std::vector<float> descriptorsValues;
            std::vector<cv::Point> locations;
            hog.compute(image, descriptorsValues, cv::Size(0,0), cv::Size(0,0), locations);

            std::cout << line;
            for(size_t i = 0; i < descriptorsValues.size(); i++) {
                std::cout << " " << descriptorsValues[i];
            }
            std::cout << "\n";
        }
    }
}
