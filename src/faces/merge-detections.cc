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

    // BFMTV_BFMStory_2012-01-10_175800 140.40 142.48 shot_16 3510 3562 3536 S18
    struct ShotCluster {
        std::string id;
        std::vector<int> frames;

        static std::map<std::string, ShotCluster> Read(std::ifstream& input) {
            std::map<std::string, ShotCluster> output;
            std::string line;
            while(std::getline(input, line)) {
                std::stringstream reader(line);
                std::string dummy, cluster_id;
                int frame;
                reader >> dummy >> dummy >> dummy >> dummy >> dummy >> dummy >> frame >> cluster_id;
                ShotCluster& cluster = output[cluster_id];
                cluster.frames.push_back(frame);
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

        static std::vector<amu::ShotSegment> ReadMerged(std::ifstream& input, const amu::Idx& idx) {
            std::vector<amu::ShotSegment> output;
            std::string line;
            int previousStart = -1, previousEnd = -1, previousFrame = -1;
            std::string previousId = "";
            while(std::getline(input, line)) {
                std::stringstream reader(line);
                std::string show, shot_id, cluster_id;
                int start_frame, end_frame, frame;
                double start_time, end_time;
                // BFMTV_BFMStory_2012-01-10_175800 140.40 142.48 shot_16 3510 3562 3536 S18
                reader >> show >> start_time >> end_time >> shot_id >> start_frame >> end_frame >> frame >> cluster_id;
                if(previousId != cluster_id && previousStart != -1) {
                    //std::cerr << previousStart << " " << previousEnd << " " << previousId << "\n";
                    output.push_back(amu::ShotSegment(previousStart, previousEnd, previousFrame, idx.GetTime(previousStart), idx.GetTime(previousEnd), idx.GetTime(previousFrame) ,0, previousId));
                    previousStart = start_frame;
                }
                if(previousStart == -1) previousStart = start_frame;
                previousEnd = end_frame;
                previousFrame = frame;
                previousId = cluster_id;
            }
            if(previousStart != -1) {
                output.push_back(amu::ShotSegment(previousStart, previousEnd, previousFrame, idx.GetTime(previousStart), idx.GetTime(previousEnd), idx.GetTime(previousFrame) ,0));
            }
            return output;
        }

        static std::vector<amu::ShotSegment> ReadMerged(const std::string& filename, const amu::Idx& idx) {
            std::ifstream input(filename.c_str());
            if(!input) {
                std::cerr << "ERROR: could not read \"" << filename << "\"\n";
                return std::vector<amu::ShotSegment>();
            } else {
                return ReadMerged(input, idx);
            }
        }

    };
}

int main(int argc, char** argv) {
    // TODO: update to keep shot id and cluster id

    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --shots <shot-file>               shot segmentation output\n");
    options.AddUsage("  --clustering <cluster-file>       shot clustering output\n");
    options.AddUsage("  --detections <detection-file>     haar detector ourput for each frame\n");
    options.AddUsage("  --idx <idx-file>                  idx for converting frames to times\n");
    options.AddUsage("  --min-detect <int>                minimum number of detections in track (default=5)\n");
    options.AddUsage("  --min-density <float>             minimum density of detections in track (default=0.5)\n");

    std::string shotFilename = options.Get<std::string>("--shots", "");
    std::string clusteringFilename = options.Get<std::string>("--clustering", "");
    std::string detectionFilename = options.Get<std::string>("--detections", "");
    std::string idxFilename = options.Get<std::string>("--idx", "");
    int minDetections = options.Get("--min-detect", 5);
    double minDensity = options.Get("--min-density", 0.5);

    if(options.Size() != 0 || (shotFilename == "" && clusteringFilename == "") || detectionFilename == "" || (clusteringFilename != "" && idxFilename == "")) options.Usage();
    std::vector<amu::ShotSegment> shots;
    if(shotFilename != "") {
        amu::ShotSegment::ParseShotSegmentation(shotFilename, shots);
    } else {
        amu::Idx idx(idxFilename);
        shots = amu::ShotCluster::ReadMerged(clusteringFilename, idx);
    }
    std::string showname = amu::ShowName(detectionFilename);

    std::map<int, std::vector<amu::Detection> > detections = amu::Detection::LoadByFrame(detectionFilename);

    typedef std::map<int, amu::Detection> Track;

    int num = 0;
    for(std::vector<amu::ShotSegment>::iterator shot = shots.begin(); shot != shots.end(); shot++) {
        std::map<int, std::vector<amu::Detection> >::iterator frame = detections.lower_bound(shot->startFrame);
        if(frame == detections.end() || frame->first < shot->startFrame || frame->first > shot->endFrame) continue;
        std::list<Track> tracks;
        while(frame != detections.end() && frame->first < shot->endFrame) {
            for(std::vector<amu::Detection>::iterator detection = frame->second.begin(); detection != frame->second.end(); detection++) {
                //if(detection->model > 0 || detection->skin < 0.3) continue;
                bool found = false;
                for(std::list<Track>::iterator track = tracks.begin(); track != tracks.end(); track++) {
                    const amu::Detection &other = track->rbegin()->second;
                    cv::Rect intersection = detection->location & other.location;
                    if(intersection == detection->location || intersection == other.location 
                            || intersection.area() > .8 * detection->location.area() || intersection.area() > .8 * other.location.area()) {
                        found = true;
                        if(track->find(detection->frame) == track->end() || (*track)[detection->frame].model > detection->model) {
                            (*track)[detection->frame] = *detection;
                        }
                        break;
                    }
                }
                if(!found) {
                    Track track;
                    track[detection->frame] = *detection;
                    tracks.push_back(track);
                }
            }
            frame++;
        }
        for(std::list<Track>::iterator track = tracks.begin(); track != tracks.end(); track++) {
            //std::cerr << track->size() << " " << shot->endFrame - shot->startFrame << " " << (double)track->size() / (shot->endFrame - shot->startFrame) << "\n";
            double density = (double)track->size() / (shot->endFrame - shot->startFrame);
            if(track->size() < minDetections || density < minDensity) continue;
            std::cout << showname << " " << shot->startTime << " " << shot->endTime << " head " << shot->id << ":Inconnu_" << num;
            for(Track::iterator iterator = track->begin(); iterator != track->end(); iterator++) {
                const amu::Detection& detection = iterator->second;
                std::cout << " || frame=" << detection.frame << " x=" << detection.location.x << " y=" << detection.location.y 
                    << " w=" << detection.location.width << " h=" << detection.location.height 
                    << " model=" << detection.model;
            }
            std::cout << "\n";
            num += 1;
        }
    }

    return 0;
}
