#include <iostream>
#include <list>

#include "repere.h"
#include "commandline.h"
#include "face.h"

namespace amu {
    struct Detection {
        int frame;
        cv::Rect location;
        double skin;
        int model;
        static std::vector<Detection> Load(const std::string& filename) {
            //1710 328 128 264 264 0.116276 2
            std::vector<Detection> output;
            std::ifstream input(filename.c_str());
            if(input) {
                std::string line;
                while(std::getline(input, line)) {
                    std::stringstream reader(line);
                    Detection detection;
                    reader >> detection.frame >> detection.location.x >> detection.location.y >> detection.location.width >> detection.location.height >> detection.skin >> detection.model;
                    if(detection.model < 2 && detection.skin > .3) 
                        output.push_back(detection);
                }
            } else {
                std::cerr << "ERROR: loading \"" << filename << "\"\n";
            }
            return output;
        }
        static std::map<int, std::vector<Detection> > LoadByFrame(const std::string& filename) {
            std::vector<Detection> detections = Load(filename);
            std::map<int, std::vector<Detection> > output;
            for(std::vector<Detection>::const_iterator i = detections.begin(); i != detections.end(); i++) {
                output[i->frame].push_back(*i);
            }
            return output;
        }
    };
}

int main(int argc, char** argv) {

    amu::CommandLine options(argv, "[options]\n");

    amu::VideoReader video;
    if(!video.Configure(options)) {
        return 1;
    }

    if(options.Size() != 0) options.Usage();

    std::map<int, std::vector<amu::Detection> > detections = amu::Detection::LoadByFrame("/dev/stdin");

    cv::Mat image;
    for(std::map<int, std::vector<amu::Detection> >::const_iterator frame = detections.begin(); frame != detections.end(); frame++) {
        video.Seek(frame->first);
        if(!video.ReadFrame(image) || image.empty()) {
            std::cerr << "ERROR: reading frame " << video.GetIndex() << "\n";
            continue;
        }
        for(std::vector<amu::Detection>::const_iterator detection = frame->second.begin(); detection != frame->second.end(); detection++) {
            cv::rectangle(image, detection->location, cv::Scalar(255, 0, 0));
        }
        cv::imshow("image", image);
        cv::waitKey(10);
    }

    return 0;
}
