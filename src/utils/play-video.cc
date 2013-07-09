#include <opencv2/opencv.hpp>

#include "video.h"
#include "commandline.h"

int main(int argc, char** argv) {
    amu::CommandLine options(argv, "[options]\n");

    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(options.Size() != 0) options.Usage();

    amu::Player player(video);
    while(player.Next()) {
        if(player.Playing()) std::cout << video.GetIndex() << " " << video.GetTime() << "\n";
        player.Show();
    }

    /*cv::Mat image;
    while(video.ReadFrame(image)) {
        std::cout << video.GetIndex() << " " << video.GetTime() << "\n";
        cv::imshow("video", image);
        cv::waitKey(10);
    }*/
}
