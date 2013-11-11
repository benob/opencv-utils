#include <iostream>
#include "repere.h"

/* This program generates an average horizontal/vertical gradient in javascript format for use in subshot annotation */

int main(int argc, char** argv) {
    if(argc != 1) {
        std::cerr << "usage: cat <image-list> | " << argv[0] << "\n";
        return 1;
    }
    std::string line;
    cv::Mat gray;
    cv::Mat horizontal;
    cv::Mat vertical;
    cv::vector<cv::Scalar> rows, columns;
    int num = 0;
    while(std::getline(std::cin, line)) {
        cv::Mat image = cv::imread(line, 1);
        cv::Mat resized;
        cv::resize(image, resized, cv::Size(1024, 576));
        cv::cvtColor(resized, gray, CV_BGR2GRAY);
        cv::Sobel(gray, horizontal, CV_16S, 1, 0, 3, 1, 0, cv::BORDER_DEFAULT );
        cv::Sobel(gray, vertical, CV_16S, 0, 1, 3, 1, 0, cv::BORDER_DEFAULT );
        horizontal = cv::abs(horizontal);
        vertical = cv::abs(vertical);
        for(int i = 0; i < horizontal.rows; i++) {
            while(columns.size() < i + 1) columns.push_back(0);
            columns[i] += cv::sum(vertical.row(i));
        }
        for(int i = 0; i < vertical.cols; i++) {
            while(rows.size() < i + 1) rows.push_back(0);
            rows[i] += cv::sum(horizontal.col(i));
        }
        num++;
    }
    std::cout << "rows = [";
    for(int i = 0; i < rows.size(); i++) std::cout << (uint64_t) (rows[i][0] / num) << ",";
    std::cout << "];\n";
    std::cout << "columns = [";
    for(int i = 0; i < columns.size(); i++) std::cout << (uint64_t) (columns[i][0] / num) << ",";
    std::cout << "];\n";
}
