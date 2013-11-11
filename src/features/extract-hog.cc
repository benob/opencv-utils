#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "repere.h"

int main(int argc, char** argv) {
    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --images-from-stdin               instead of reading a video, use image paths from stdin\n");
    options.AddUsage("  --shots <shots-file>              shot segmentation (required when using a video)\n");

    bool imagesFromStdin = options.IsSet("--images-from-stdin");

    cv::HOGDescriptor hog(cv::Size(128, 64), cv::Size(16,16), cv::Size(8,8), cv::Size(8,8), 9);

    if(imagesFromStdin) {
        std::string line;
        while(std::getline(std::cin, line)) {
            cv::Mat image = cv::imread(line, 0);
            if(image.rows == 0) {
                std::cerr << "ERROR: reading \"" << line << "\"\n";
                return 1;
            } else {
                cv::resize(image, image, cv::Size(128, 64));

                std::vector<float> descriptorsValues;
                std::vector<cv::Point> locations;
                hog.compute(image, descriptorsValues, cv::Size(0,0), cv::Size(0,0), locations);

                std::cout << line;
                for(size_t i = 0; i < descriptorsValues.size(); i++) {
                    std::cout << " " << descriptorsValues[i];
                }
                std::cout << "\n";
            }
        }
    } else {
        std::string shotFile = options.Get<std::string>("--shots", "");

        amu::VideoReader video;
        if(!video.Configure(options)) return 1;
        if(options.Size() != 0 || shotFile == "") options.Usage();

        std::fstream shotStream(shotFile.c_str());
        if(!shotStream) {
            std::cerr << "ERROR: cannot read template file \"" << shotFile << "\"\n";
            return 1;
        }
        std::vector<amu::ShotSegment> shots;
        amu::ShotSegment::ParseShotSegmentation(shotStream, shots);

        cv::Mat image;
        for(size_t shot = 0; shot < shots.size(); shot++) {
            video.Seek(shots[shot].frame);
            video.ReadFrame(image);
            cv::resize(image, image, cv::Size(128, 64));

            std::vector<float> descriptorsValues;
            std::vector<cv::Point> locations;
            hog.compute(image, descriptorsValues, cv::Size(0,0), cv::Size(0,0), locations);

            std::cout << shots[shot].frame;
            for(size_t i = 0; i < descriptorsValues.size(); i++) {
                std::cout << " " << descriptorsValues[i];
            }
            std::cout << "\n";
        }
    }
    return 0;
}
