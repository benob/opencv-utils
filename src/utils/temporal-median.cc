#include <iostream>
#include <list>
#include <cv.h>
#include <highgui.h>
#include <libgen.h>

#include "ffmpeg_fas.h"
#include "video.h"

#include "shot.h"

int main(int argc, char** argv) {
    amu::CommandLine options(argv);
    amu::VideoReader video(argv[1]);
    if(!video.Configure(options)) return 1;
    if(options.Size() == 0) options.Usage();

    amu::ShotSegmenter shots;

    cv::Mat frame;
    int lastBoundary = 0;
    video.Seek(3000);
    int width = 1024 / 4, height = 576 / 4;
    int num_frames = 20;
    cv::Mat images[num_frames];
    int c1[num_frames], c2[num_frames], c3[num_frames], c4[num_frames];
    cv::Mat output(cv::Size(width, height), CV_8UC3);
    while (video.HasNext()) {
        video.ReadFrame(frame);
        if(shots.IsBoundary(frame)) {
            int boundary = video.GetIndex();
            if(lastBoundary > 0) {
                for(int i = 0; i < num_frames; i++) {
                    int where = lastBoundary + (i * (boundary - lastBoundary) / (num_frames + 1));
                    video.Seek(where);
                    video.ReadFrame(frame);
                    frame.copyTo(images[i]);
                }
                std::cerr << frame.cols << "x" << frame.rows << "\n";
                for(int x = 0; x < height; x++) {
                    for(int y = 0; y < width; y++) {
                        for(int i = 0; i < num_frames; i++) {
                            uint32_t color = images[i].at<uint32_t>(x, y); 
                            c1[i] = (color & 0xff);
                            c2[i] = ((color >> 8) & 0xff);
                            c3[i] = ((color >> 16) & 0xff);
                            c4[i] = ((color >> 24) & 0xff);
                        }
                        std::sort(c1, c1 + num_frames);
                        std::sort(c2, c2 + num_frames);
                        std::sort(c3, c3 + num_frames);
                        std::sort(c4, c4 + num_frames);
                        int middle = num_frames / 2;
                        uint32_t color = c1[middle] | (c2[middle] << 8) | (c3[middle] << 16) | (c4[middle] << 24);
                        output.at<uint32_t>(x, y) = color;
                    }
                }
                for(int i = 0; i < num_frames; i++) {
                    std::stringstream name;
                    name << "input" << i;
                    cv::imshow(name.str(), images[i]);
                }
                cv::imshow("median", output);
                cv::waitKey(0);
            }
            lastBoundary = boundary;
        }
        //cv::imshow("frame", frame);
    }
    return 0;
}
