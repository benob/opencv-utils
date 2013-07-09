#pragma once

#include <opencv2/opencv.hpp>

namespace amu {

    struct ShotSegmenter {
        cv::Mat previous;
        cv::vector<cv::Point2f> points, newPoints, matchedPoints;
        size_t numPoints;

        bool IsBoundary(const cv::Mat& image, double threshold = 30) {
            cv::Mat gray;
            cv::cvtColor(image, gray, CV_BGR2GRAY);
            if(points.size() == 0 || points.size() < numPoints / 2) {
                cv::goodFeaturesToTrack(gray, points, 200, 0.01, image.cols / 25);
                numPoints = points.size();
                gray.copyTo(previous);
                return true;
            } else {
                newPoints.clear();
                std::vector<uchar> status;
                std::vector<float> error;
                cv::calcOpticalFlowPyrLK(previous, gray, points, newPoints, status, error);
                matchedPoints.clear();
                double cumulativeError = 0;
                for(size_t i = 0; i < newPoints.size(); i++) {
                    if(status[i] == 1) {
                        cumulativeError += error[i];
                        matchedPoints.push_back(newPoints[i]);
                    }
                }
                points = matchedPoints;
                cumulativeError /= matchedPoints.size();
                if(!(cumulativeError < threshold)) {
                    points.clear();
                }
                gray.copyTo(previous);
                return false;
            }
        }
    };

    struct ShotRecognizer {
        cv::vector<cv::Mat> images;
        cv::vector<cv::vector<cv::Point2f> > points;
        int ShotId(const cv::Mat& image, double threshold = 10) {
            cv::Mat gray;
            cv::cvtColor(image, gray, CV_BGR2GRAY);
            int argmin = -1;
            double min = 0;
            for(size_t model = 0; model < points.size(); model++) {
                cv::vector<cv::Point2f> matches;
                std::vector<uchar> status;
                std::vector<float> error;
                cv::calcOpticalFlowPyrLK(images[model], gray, points[model], matches, status, error);
                double cumulativeError = 0;
                size_t numMatches = 0;
                for(size_t i = 0; i < matches.size(); i++) {
                    if(status[i] == 1) {
                        cumulativeError += error[i];
                        numMatches++;
                    }
                }
                cumulativeError /= numMatches;
                if(numMatches > matches.size() / 2 && (argmin == -1 || cumulativeError < min)) {
                    min = cumulativeError;
                    argmin = model;
                }
            }
            if(argmin == -1 || min > threshold) {
                argmin = points.size();
                images.push_back(cv::Mat(gray));
                points.push_back(cv::vector<cv::Point2f>());
                cv::goodFeaturesToTrack(images[argmin], points[argmin], 200, 0.01, gray.cols / 25);
            }
            return argmin;
        }
    };
}
