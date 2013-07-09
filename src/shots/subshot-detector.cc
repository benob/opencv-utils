#include <opencv2/opencv.hpp>
#include <iostream>
#include "nbest.h"

inline int column_integral(const cv::Mat& sum, const int x, const int y) {
    return sum.at<int>(y, x + 1) - sum.at<int>(y, x);
}

inline int row_integral(const cv::Mat& sum, const int x, const int y) {
    return sum.at<int>(y + 1, x) - sum.at<int>(y, x);
}

double min(double x1, double x2) {
    return x1 < x2 ? x1 : x2;
}

double min_ratio(double x1, double x2, double x3, double x4) {
    return min(x1, min(x2, min(x3, x4)));
}

double FindRectangles(cv::Mat& edges, cv::Mat& image) {
    cv::imshow("edges", edges);
    cv::Mat sum;
    int width = edges.cols;
    int height = edges.rows;
    cv::integral(edges, sum);

    amu::StdNBest vertical(10);
    for(int x = 0; x < width; x++) {
        vertical.insertNmax(column_integral(sum, x, height - 1), x);
    }
    vertical.insertNmax(height, 0);
    vertical.insertNmax(height, width - 1);
    vertical.sortNmax();

    amu::StdNBest horizontal(10);
    for(int y = 0; y < height; y++) {
        horizontal.insertNmax(row_integral(sum, width - 1, y), y);
    }
    horizontal.insertNmax(width, 0);
    horizontal.insertNmax(width, height - 1);
    horizontal.sortNmax();

    /*for(int i = 0; i < vertical.size(); i++) {
        cv::line(image, cv::Point(vertical.get(i), 0), cv::Point(vertical.get(i), height), cv::Scalar(0, 0, 255), 1);
    }
    for(int j = 0; j < horizontal.size(); j++) {
        cv::line(image, cv::Point(0, horizontal.get(j)), cv::Point(width, horizontal.get(j)), cv::Scalar(255, 0, 0), 1);
    }*/

    std::vector<int> sorted_vertical;
    for(int i = 0; i < vertical.size(); i++) sorted_vertical.push_back(vertical.get(i));
    std::sort(sorted_vertical.begin(), sorted_vertical.end());

    std::vector<int> sorted_horizontal;
    for(int i = 0; i < horizontal.size(); i++) sorted_horizontal.push_back(horizontal.get(i));
    std::sort(sorted_horizontal.begin(), sorted_horizontal.end());

    for(size_t i = 0; i < sorted_vertical.size(); i++) std::cout << sorted_vertical[i] << " "; std::cout << "\n";
    for(size_t i = 0; i < sorted_horizontal.size(); i++) std::cout << sorted_horizontal[i] << " "; std::cout << "\n";

    double max = 0;
    cv::Rect argmax;
    int numRects = 0;
    for(size_t i = 0; i < sorted_vertical.size() - 1; i++) {
        for(size_t j = i + 1; j < sorted_vertical.size(); j++) {
            for(size_t k = 0; k < sorted_horizontal.size() - 1; k++) {
                for(size_t l = k + 1; l < sorted_horizontal.size(); l++) {
                    int x1 = sorted_vertical[i], x2 = sorted_vertical[j];
                    int y1 = sorted_horizontal[k], y2 = sorted_horizontal[l];
                    int perimeter = 2 * (x2 - x1) + 2 * (y2 - y1);

                    int border1 = row_integral(sum, x2, y1) - row_integral(sum, x1, y1);
                    int border2 = row_integral(sum, x2, y2) - row_integral(sum, x1, y2);
                    int border3 = column_integral(sum, x1, y2) - column_integral(sum, x1, y1);
                    int border4 = column_integral(sum, x2, y2) - column_integral(sum, x2, y1);

                    double ratio1 = (double) border1 / (256.0 * (x2 - x1));
                    double ratio2 = (double) border2 / (256.0 * (x2 - x1));
                    double ratio3 = (double) border3 / (256.0 * (y2 - y1));
                    double ratio4 = (double) border4 / (256.0 * (y2 - y1));

                    /*int border = row_integral(sum, x2, y1) - row_integral(sum, x1, y1)
                        + row_integral(sum, x2, y2) - row_integral(sum, x1, y2)
                        + column_integral(sum, x1, y2) - column_integral(sum, x1, y1)
                        + column_integral(sum, x2, y2) - column_integral(sum, x2, y1);

                    double ratio = (double) border / perimeter;*/

                    double ratio = (ratio1 + ratio2 + ratio3 + ratio4) / 4;

                    int area = (x2 - x1) * (y2 - y1);
                    if(ratio > 0.5 /*&& area > (width / 10) * (height / 10)*/ && min_ratio(ratio1, ratio2, ratio3, ratio4) > 0.5) {
                        numRects++;
                        cv::rectangle(image, cv::Rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1), cv::Scalar(0, 255, 0), 1);
                        //std::cout << border1 / 256.0 << " " << border2 / 256.0 << " " << border3 / 256.0 << " " << border4 / 256.0 << "\n";
                        std::cout << ratio << " " << ratio1 << " "<< ratio2 << " "<< ratio3 << " "<< ratio4 << " "    << cv::Rect(x1, y1, x2 - x1, y2 - y1) << "\n";

                        if(ratio > max) {
                            max = ratio;
                            argmax = cv::Rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1);
                        }
                    }
                }
            }
        }
    }
    std::cout << numRects << " " << max << " " << argmax << "\n";

    cv::imshow("image", image);
    cv::waitKey(0);
    return 0;
}

int main(int argc, char** argv) {

    cv::Mat accu(cv::Size(1024, 576), CV_32F);
    accu.setTo(0);

    cv::Mat image;
    for(int i = 1; i < argc; i++) {
        image = cv::imread(argv[i], 1);
        cv::resize(image, image, cv::Size(1024, 576));
        cv::Mat edges, gray;
        cv::cvtColor(image, gray, CV_BGR2GRAY);
        cv::Canny(gray, edges, 150, 200, 3);
        cv::accumulate(edges & 1, accu);
    }

    accu /= argc - 1;
    
    //imshow("accu", accu);
    cv::Mat accu2(cv::Size(1024, 576), CV_8UC1);
    cv::normalize(accu, accu2, 0, 255, cv::NORM_MINMAX, CV_8UC1);
    FindRectangles(accu2, image);
    cv::waitKey(0);
    /*cv::vector<cv::Vec4i> lines;
      cv::HoughLinesP(edges, lines, 1, CV_PI/2, 50, 50, 4);
      for( size_t i = 0; i < lines.size(); i++ )
      {
      cv::Vec4i l = lines[i];
      cv::line(image, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), cv::Scalar(0,0,255), 1, CV_AA);
      }
      cv::imshow("image", image);
      cv::waitKey(0);*/
}
