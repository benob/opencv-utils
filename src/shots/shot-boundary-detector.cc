#include <iostream>
#include <algorithm>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "buffer.h"
#include "repere.h"
#include "commandline.h"

void BGRhistogram(const cv::Mat& image, cv::Mat& histogram) {
    int numImages = 1;
    int channels[] = {0, 1, 2};
    int dimensions = 3;
    int histogramSize[] = {8, 8, 8};
    float hRanges[] = {0, 180};
    float sRanges[] = {0, 256};
    float vRanges[] = {0, 256};
    const float* ranges[] = {hRanges, sRanges, vRanges};
    cv::calcHist(&image, numImages, channels, cv::Mat(), histogram, dimensions, histogramSize, ranges, true);
    cv::normalize(histogram, histogram);
}

void HSVhistogram(const cv::Mat& image, cv::Mat& histogram) {
    static cv::Mat hsv;
    cv::cvtColor(image, hsv, CV_BGR2HSV);
    BGRhistogram(hsv, histogram);
}

void DirectionHistogram(const cv::Mat& image, cv::Mat& histogram) {
    cv::Mat gray, gradients[2], gradient;
    cv::cvtColor(image, gray, CV_RGB2GRAY);
    cv::Scharr(gray, gradients[0], CV_16S, 1, 0, 1, 0, cv::BORDER_DEFAULT);
    cv::Scharr(gray, gradients[1], CV_16S, 0, 1, 1, 0, cv::BORDER_DEFAULT);
    cv::merge(gradients, 2, gradient);
    cv::convertScaleAbs(gradient, gradient);

    int numImages = 1;
    int channels[] = {0, 1};
    int dimensions = 2;
    int histogramSize[] = {8, 8};
    float xRanges[] = {0, 256};
    float yRanges[] = {0, 256};
    const float* ranges[] = {xRanges, yRanges};
    cv::calcHist(&gradient, numImages, channels, cv::Mat(), histogram, dimensions, histogramSize, ranges, true);
    cv::normalize(histogram, histogram);
}

double ManhattanDistance(const cv::Mat& a, const cv::Mat& b) {
    return cv::sum(cv::abs(a - b))[0];
}

double AverageDistanceCut(const amu::Buffer<cv::Mat>& histogram) {
    double sum = 0;
    int num = histogram.size();
    for(int i = 0; i < num / 2; i++) {
        for(int j = num / 2; j < num; j++) {
            sum += ManhattanDistance(histogram[i], histogram[j]);
        }
    }
    return sum / ((num / 2)* (num / 2));
}

double Median(const amu::Buffer<double>& values) {
    int num = values.size();
    double sorted[num];
    for(int i = 0; i < num; i++) {
        sorted[i] = values[i];
    }
    size_t n = num / 2;
    //std::nth_element(&sorted[0], &sorted[n], &sorted[num]);
    std::sort(&sorted[0], &sorted[num]);
    return sorted[n];
}

int main(int argc, char** argv) {

    amu::CommandLine options(argv, "[options]\n");

    amu::VideoReader video;
    if(!video.Configure(options)) {
        return 1;
    }

    if(options.Size() != 0) options.Usage();

    int window = 9;
    int factor = 2;
    double threshold = 1;

    amu::Buffer<cv::Mat> histogram(window); // histograms
    amu::Buffer<std::pair<int, double> > frameNum(window); // frame number corresponding to an histogram

    amu::Buffer<double> distances(window * 8); // histogram distance
    amu::Buffer<std::pair<int, double> > indices(window * 8); // frame number corresponding to distances

    cv::Mat original;
    bool aboveMedian = false;
    double max = 0;
    std::pair<int, double> argmax;
    std::pair<int, double> lastFrame;
    std::pair<int, double> lastVideoFrame(-1, 0);
    cv::Mat image;

    while(video.HasNext() || distances.size() > 0) {
        if(video.ReadFrame(image)) {
            if(lastVideoFrame.first != -1 && video.GetIndex() - lastVideoFrame.first > 10) { // reset
                argmax = lastVideoFrame;
                std::cout << lastFrame.first << " " << argmax.first << " " << (lastFrame.first + argmax.first) / 2  << " "
                    << lastFrame.second << " " << argmax.second << " " << (lastFrame.second + argmax.second) / 2
                    << " " << max << "\n" << std::flush;
                lastFrame = std::pair<int, double>(video.GetIndex(), video.GetTime());
                histogram.clear();
                frameNum.clear();
                distances.clear();
                indices.clear();
                aboveMedian = false;
                max = 0;
            }
            frameNum.push(std::pair<int, double>(video.GetIndex(), video.GetTime()));
            lastVideoFrame = frameNum[-1];

            HSVhistogram(image, histogram.push());
            distances.push(AverageDistanceCut(histogram));

            indices.push(frameNum[frameNum.size() / 2]);

        } else {
            distances.shift();
            indices.shift();
        }
        double median = Median(distances);
        std::pair<int, double> index = indices[distances.size() / 2];
        double distance = distances[distances.size() / 2];
        if(!aboveMedian && distance > median * factor && distance > threshold) {
            aboveMedian = true;
            max = distance;
            argmax = index;
        }
        if(aboveMedian) {
            if(distance > max) {
                max = distance;
                argmax = index;
            }
            if(distance <= median * 2 || distance < threshold) {
                aboveMedian = false;
                std::cout << lastFrame.first << " " << argmax.first << " " << (lastFrame.first + argmax.first) / 2 << " "
                    << lastFrame.second << " " << argmax.second << " " << (lastFrame.second + argmax.second) / 2
                    << " " << max << "\n" << std::flush;
                lastFrame = argmax;
            }
        }
    }
    if(lastVideoFrame != lastFrame) {
        argmax = lastVideoFrame;
        std::cout << lastFrame.first << " " << argmax.first << " " << (lastFrame.first + argmax.first) / 2  << " "
            << lastFrame.second << " " << argmax.second << " " << (lastFrame.second + argmax.second) / 2
            << " " << max << "\n" << std::flush;
    }

    return 0;
}
