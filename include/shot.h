#pragma once

#include <iostream>
#include <opencv2/opencv.hpp>

namespace amu {

    struct ShotSegment {
        int startFrame;
        int endFrame;
        int frame;
        double startTime;
        double endTime;
        double time;
        double score;
        std::string id;
        ShotSegment(int _startFrame, int _endFrame, int _frame, double _startTime, double _endTime, double _time, double _score, const std::string& _id = "") : startFrame(_startFrame), endFrame(_endFrame), frame(_frame), startTime(_startTime), endTime(_endTime), time(_time), score(_score), id(_id) { }
        static std::vector<ShotSegment> Read(std::istream& input) {
            std::vector<ShotSegment> output;
            std::string line;
            while(std::getline(input, line)) {
                std::stringstream reader(line);
                std::string dummy, id;
                int startFrame, endFrame, frame; 
                double startTime, endTime, time;
                double score;
                reader >> dummy >> dummy >> dummy >> dummy >> id >> startFrame >> endFrame >> frame >> startTime >> endTime >> time >> score;
                ShotSegment shot(startFrame, endFrame, frame, startTime, endTime, time, score, id);
                output.push_back(shot);
            }
            return output;
        }

        static std::vector<ShotSegment> Read(const std::string& filename) {
            std::ifstream input(filename.c_str());
            if(!input) {
                std::cerr << "ERROR: loading shots from \"" << filename << "\"\n";
                return std::vector<ShotSegment>();
            }
            return Read(input);
        }

        static bool ParseShotSegmentation(std::istream& input, std::vector<ShotSegment>& output) {
            std::string line;
            while(std::getline(input, line)) {
                std::stringstream reader(line);
                int startFrame, endFrame, frame; 
                double startTime, endTime, time;
                double score;
                reader >> startFrame >> endFrame >> frame >> startTime >> endTime >> time >> score;
                ShotSegment shot(startFrame, endFrame, frame, startTime, endTime, time, score);
                output.push_back(shot);
            }
            return true;
        }
        static bool ParseShotSegmentation(const std::string& filename, std::vector<ShotSegment>& output) {
            std::ifstream input(filename.c_str());
            if(!input) {
                std::cerr << "ERROR: loading shots from \"" << filename << "\"\n";
                return false;
            }
            return ParseShotSegmentation(input, output);
        }

    };

    struct SubShot {
        int x;
        int y;
        int width;
        int height;
        SubShot(int _x, int _y, int _width, int _height) : x(_x), y(_y), width(_width), height(_height) { }
    };

    struct Split {
        std::string type;
        std::vector<amu::SubShot> subshots;
        Split(const std::string& _type) : type(_type) { }
        void Add(const SubShot& subshot) {
            subshots.push_back(subshot);
        }
        static bool ParseTemplates(std::istream& input, std::vector<Split>& output, double scale = 1.0) {
            std::string line;
            while(std::getline(input, line)) {
                std::stringstream reader(line);
                std::string type;
                int numSubshots;
                reader >> type >> numSubshots;
                amu::Split split(type);
                for(int i = 0; i < numSubshots; i++) {
                    int x, y, width, height;
                    reader >> x >> y >> width >> height;
                    split.Add(SubShot(x * scale, y * scale, width * scale, height * scale));
                }
                output.push_back(split);
            }
            return true;
        }
        static bool ParseTemplates(const std::string& filename, std::vector<Split>& output, double scale = 1.0) {
            std::ifstream input(filename.c_str());
            if(!input) {
                std::cerr << "ERROR: loading templates from \"" << filename << "\"\n";
                return false;
            }
            return ParseTemplates(input, output, scale);
        }

    };

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

    struct TemplateMatcher {
        static int Match(cv::Mat& image, std::vector<amu::Split>& templates, const std::string& type = "", int iterations = 100) {
            cv::Mat gray, horizontal, vertical;
            cv::cvtColor(image, gray, CV_BGR2GRAY);
            cv::Sobel(gray, horizontal, CV_16S, 1, 0, 3, 1, 0, cv::BORDER_DEFAULT );
            cv::Sobel(gray, vertical, CV_16S, 0, 1, 3, 1, 0, cv::BORDER_DEFAULT );
            horizontal = cv::abs(horizontal);
            vertical = cv::abs(vertical);
            //cv::threshold(horizontal, horizontal, 192, 255, CV_THRESH_BINARY);
            //cv::threshold(vertical, vertical, 192, 255, CV_THRESH_BINARY);

            horizontal.convertTo(horizontal, CV_8U);
            vertical.convertTo(vertical, CV_8U);

            int argmax = -1;
            double max = 0;
            for(size_t i = 0; i < templates.size(); i++) {
                const amu::Split& split = templates[i];
                if(type != "" && split.type != type) continue;
                for(int iteration = 0; iteration < 100; iteration++) {
                    double sum = 0; // sum of gradients over rectangles
                    double norm = 0;
                    for(size_t j = 0; j < split.subshots.size(); j++) {
                        amu::SubShot subshot = split.subshots[j]; // copy to modify

                        // randomily move bounds
                        if(iteration != 0) {
                            subshot.x += rand() % 3 - 1;
                            subshot.y += rand() % 3 - 1;
                            subshot.width += rand() % 3 - 1;
                            subshot.height += rand() % 3 - 1;
                        }

                        for(int x = subshot.x; x < subshot.x + subshot.width; x++) {
                            sum += vertical.at<uchar>(subshot.y, x);
                            sum += vertical.at<uchar>(subshot.y + subshot.height, x);
                            //sum -= horizontal.at<uchar>(subshot.y, x);
                            //sum -= horizontal.at<uchar>(subshot.y + subshot.height, x);
                        }
                        for(int y = subshot.y; y < subshot.y + subshot.height; y++) {
                            sum += horizontal.at<uchar>(y, subshot.x);
                            sum += horizontal.at<uchar>(y, subshot.x + subshot.width);
                            //sum -= vertical.at<uchar>(y, subshot.x);
                            //sum -= vertical.at<uchar>(y, subshot.x + subshot.width);
                        }
                        norm += subshot.width * 2 + subshot.height * 2;
                    }
                    sum /= 256 * norm;
                    if(sum > max) {
                        //std::cout << i << " " << sum << "\n";
                        max = sum;
                        argmax = i;
                    }
                }

            }
            return argmax;
        }

    };

}
