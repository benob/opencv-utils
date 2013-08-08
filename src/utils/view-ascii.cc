#include "repere.h"

inline int bgr2ansi(cv::Vec3b color) {
    if(fabs(color[2] - color[1]) + fabs(color[2] - color[0]) + fabs(color[1] - color[0]) < 12)
        return (color[0] + color[1] + color[2]) * 24 / (3 * 256) + 232;
    int r2 = color[2] * 6 / 256;
    int g2 = color[1] * 6 / 256;
    int b2 = color[0] * 6 / 256;
    return r2 * 36 + g2 * 6 + b2 + 16;
}

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cerr << "usage: " << argv[0] << " <images>\n";
        return 1;
    }
    for(int i = 1; i < argc; i++) {
        std::cout << argv[i] << "\n";
        cv::Mat image = cv::imread(argv[i]);
        if(image.cols == 0 || image.rows == 0) continue;
        cv::Mat resized;
        int width = 80, height = 40;
        if(image.cols > image.rows) {
            height = image.rows * width / (image.cols);
        } else {
            width = image.cols * height / (image.rows);
        }
        cv::resize(image, resized, cv::Size(width, height), cv::INTER_AREA);
        for(int y = 0; y < height / 2; y++) {
            for(int x = 0; x < width; x++) {
                std::cout << "\033[48;5;" << bgr2ansi(resized.at<cv::Vec3b>(y * 2, x)) << "m";
                std::cout << "\033[38;5;" << bgr2ansi(resized.at<cv::Vec3b>(y * 2 + 1, x)) << "m▄";
            }
            std::cout << "\033[0m\n";
        }
    }
}
