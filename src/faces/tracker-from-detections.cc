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
                    if(detection.model == 0 && detection.skin > .3) output.push_back(detection);
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

    double scale = options.Read("--scale", 1.0);
    std::string shotFilename = options.Get<std::string>("--shots", "");
    std::string detectionFilename = options.Get<std::string>("--detections", "");

    amu::VideoReader video;
    if(!video.Configure(options)) {
        return 1;
    }

    //if(options.Size() != 0 || shotFilename == "" || detectionFilename == "") options.Usage();

    /*std::vector<amu::ShotSegment> shots;
    amu::ShotSegment::ParseShotSegmentation(shotFilename, shots);

    std::map<int, std::vector<amu::Detection> > detections = amu::Detection::LoadByFrame(detectionFilename);
    */

    cv::Mat image;

    /*for(size_t shot = 20; shot < shots.size(); shot++) {
        int frame = shots[shot].frame;
        video.Seek(frame);
        cv::Mat gray;
        cv::cvtColor(image, gray, CV_BGR2GRAY);
        std::vector<amu::Tracker> trackers;
        for(size_t face = 0; face < detections[frame].size(); face++) {
            const amu::Detection& detection = detections[frame][face];
            amu::Tracker tracker;
            tracker.AddModel(gray, detection.location);
            trackers.push_back(tracker);
            cv::rectangle(image, detection.location, cv::Scalar(0, 255, 0), 1);
            tracker.Draw(image);
        }
        cv::imshow("image", image);
        cv::waitKey(0);
        */

    cv::CascadeClassifier detector("data/haarcascades/haarcascade_frontalface_alt.xml");
    std::list<amu::Tracker> trackers;
    cv::Mat gray;
    bool once = true;
    while(video.HasNext()) {
        video.ReadFrame(image);
        cv::cvtColor(image, gray, CV_BGR2GRAY);

        for(std::list<amu::Tracker>::iterator i = trackers.begin(); i != trackers.end(); i++) {
            if(i->Find(gray)) {
                i->Draw(image);
            } else {
                i = trackers.erase(i);
            }
        }

        if(video.GetIndex() % 5 == 0) {
        //if(once) {
            once = false;
            std::vector<cv::Rect> faces;
            detector.detectMultiScale(image, faces);
            for(size_t i = 0; i < faces.size(); i++) {
                if(amu::Skin(image, faces[i]) > .3) {
                    //amu::Shrink(faces[i], .5);
                    bool found = false;
                    for(std::list<amu::Tracker>::iterator t = trackers.begin(); t != trackers.end(); t++) {
                        if(t->Matches(faces[i])) {
                            found = true;
                            t->AddModel(gray, faces[i]);
                            break;
                        }
                    }
                    if(!found) {
                        amu::Tracker tracker;
                        tracker.AddModel(gray, faces[i]);
                        trackers.push_back(tracker);
                        tracker.Draw(image);
                        cv::rectangle(image, faces[i], cv::Scalar(255, 0, 0), 1);
                    }
                }
            }
        }
        std::cout << trackers.size() << "\n";

        cv::imshow("image", image);
        cv::waitKey(0);
    }

    return 0;
}
