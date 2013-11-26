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
    options.AddUsage("  --shots <shot-file>               shot segmentation output\n");
    options.AddUsage("  --detections <detection-file>     haar detector ourput for each frame\n");

    std::string shotFilename = options.Get<std::string>("--shots", "");
    std::string detectionFilename = options.Get<std::string>("--detections", "");

    if(options.Size() != 0 || shotFilename == "" || detectionFilename == "") options.Usage();

    std::vector<amu::ShotSegment> shots;
    amu::ShotSegment::ParseShotSegmentation(shotFilename, shots);

    std::map<int, std::vector<amu::Detection> > detections = amu::Detection::LoadByFrame(detectionFilename);

    int num = 0;
    std::string showname = amu::ShowName(shotFilename);
    for(std::vector<amu::ShotSegment>::iterator shot = shots.begin(); shot != shots.end(); shot++) {
        std::map<int, std::vector<amu::Detection> >::iterator frame = detections.find(shot->startFrame);
        std::list<std::vector<amu::Detection> > tracks;
        while(frame != detections.end() && frame->first < shot->endFrame) {
            for(std::vector<amu::Detection>::iterator detection = frame->second.begin(); detection != frame->second.end(); detection++) {
                //if(detection->model > 0 || detection->skin < 0.3) continue;
                bool found = false;
                for(std::list<std::vector<amu::Detection> >::iterator track = tracks.begin(); track != tracks.end(); track++) {
                    const amu::Detection &other = track->back();
                    cv::Rect intersection = detection->location & other.location;
                    if(intersection == detection->location || intersection == other.location 
                            || intersection.area() > .8 * detection->location.area() || intersection.area() > .8 * other.location.area()) {
                        found = true;
                        track->push_back(*detection);
                        break;
                    }
                }
                if(!found) {
                    std::vector<amu::Detection> track;
                    track.push_back(*detection);
                    tracks.push_back(track);
                }
            }
            frame++;
        }
        for(std::list<std::vector<amu::Detection> >::iterator track = tracks.begin(); track != tracks.end(); track++) {
            std::cout << showname << " " << shot->startTime << " " << shot->endTime << " head Inconnu_" << num;
            for(std::vector<amu::Detection>::iterator detection = track->begin(); detection != track->end(); detection++) {
                std::cout << " || frame=" << detection->frame << " x=" << detection->location.x << " y=" << detection->location.y 
                    << " width=" << detection->location.width << " height=" << detection->location.height 
                    << " model=" << detection->model;
            }
            std::cout << "\n";
            num += 1;
        }
    }

    return 0;
}
