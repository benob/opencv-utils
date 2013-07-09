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

int main(int argc, char** argv) {

    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --time                            specify that shot boundaries are in seconds\n");
    bool isTime = options.IsSet("--time");

    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(options.Size() != 0) options.Usage();

    std::string line;
    while(std::getline(std::cin, line)) {
        std::stringstream reader(line);
        double frame; double similarity;
        reader >> frame >> similarity;
        std::cout << line << "\n";
        ShowTiles(video, frame, isTime);
    }

    return 0;
}
