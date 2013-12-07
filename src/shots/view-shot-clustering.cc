#include <iostream>
#include <algorithm>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "video.h"

void ShowTiles(amu::VideoReader& video, double frame, bool isTime = false) {
    if(isTime) {
        video.SeekTime(frame);
        if(fabs(frame - video.GetTime()) > 1) {
            std::cerr << "WARNING: Could not seek to time " << frame << "\n";
            return;
        }
        frame = video.GetIndex() - 1;
        std::cout << frame << "\n";
    }
    if(frame > 3) video.Seek((int) (frame - 4));
    if(fabs(frame - video.GetIndex()) > 25) {
        std::cerr << "WARNING: Could not seek to frame " << frame << "\n";
        return;
    }
    cv::Mat current;
    cv::Size size = video.GetSize();
    cv::Mat image(size * 3, CV_8UC3);
    for(int x = 0; x < 3; x++) {
        for(int y = 0; y < 3; y++) {
            cv::Mat target(image, cv::Range(x * size.height, (x + 1) * size.height), cv::Range(y * size.width, (y + 1) * size.width));
            if(video.ReadFrame(current)) {
                cv::resize(current, target, size);
            } else {
                target.setTo(0);
            }
            if((int) frame == video.GetIndex()) {
                cv::rectangle(image, cv::Rect(y * size.width, x * size.height, size.width, size.height), cv::Scalar(0, 0, 255));
            }
        }
    }
    cv::imshow("tiles", image);
    cv::waitKey(-1);
}

namespace amu {
    // file 1 0 6182 U U U S1
    struct ShotCluster {
        std::string id;
        std::vector<int> frames;

        static std::map<std::string, ShotCluster> Read(std::ifstream& input) {
            std::map<std::string, ShotCluster> output;
            std::string line;
            while(std::getline(input, line)) {
                std::stringstream reader(line);
                std::string dummy, id;
                int start, duration;
                reader >> dummy >> dummy >> start >> duration >> dummy >> dummy >> dummy >> id;
                ShotCluster& cluster = output[id];
                cluster.frames.push_back(start + (int) (duration / 2));
            }
            return output;
        }

        static std::map<std::string, ShotCluster> Read(const std::string& filename) {
            std::ifstream input(filename.c_str());
            if(!input) {
                std::cerr << "ERROR: could not read \"" << filename << "\"\n";
                return std::map<std::string, ShotCluster>();
            } else {
                return Read(input);
            }
        }
    };
}

int main(int argc, char** argv) {

    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --clustering <filename>           specify that shot boundaries are in seconds\n");
    std::string clusteringFilename = options.Get<std::string>("--clustering", "");

    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(options.Size() != 0 || clusteringFilename == "") options.Usage();

    std::map<std::string, amu::ShotCluster> clusters = amu::ShotCluster::Read(clusteringFilename);

    int maxFrames = 0;
    for(std::map<std::string, amu::ShotCluster>::iterator cluster = clusters.begin(); cluster != clusters.end(); cluster++) {
        if(cluster->second.frames.size() > maxFrames) maxFrames = cluster->second.frames.size();
    }

    cv::Size size = video.GetSize();
    int width = 8;
    int height = (clusters.size() / width) + 1;
    int tileWidth = size.width / 4;
    int tileHeight = size.height / 4;

    cv::Mat result(cv::Size(tileWidth * width, tileHeight * height), CV_8UC3);
    int shotNum = 0;
    for(std::map<std::string, amu::ShotCluster>::iterator cluster = clusters.begin(); cluster != clusters.end(); cluster++) {
        cv::Mat image;
        for(std::vector<int>::iterator frame = cluster->second.frames.begin(); frame != cluster->second.frames.end(); frame++) {
            std::cout << cluster->first << " " << *frame << "\n";
            video.Seek(*frame);
            video.ReadFrame(image);
            cv::Mat target(result, cv::Rect((shotNum % width) * tileWidth, (shotNum / width) * tileHeight, tileWidth, tileHeight));
            cv::resize(image, target, cv::Size(tileWidth, tileHeight), cv::INTER_AREA);
            cv::rectangle(result, cv::Rect((shotNum % width) * tileWidth, (shotNum / width) * tileHeight, tileWidth, tileHeight), cv::Scalar(0, 0, 0), 1);
            shotNum++;
            cv::imshow("clustering", result);
            cv::waitKey(10);
        }
        cv::rectangle(result, cv::Rect((shotNum % width) * tileWidth - 4, (shotNum / width) * tileHeight, 2, tileHeight), cv::Scalar(0, 0, 255), 2);
    }
    cv::imshow("clustering", result);
    cv::waitKey(0);

    return 0;
}
