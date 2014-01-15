#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "shot-features.h"
#include "classify.h"

int main(int argc, char** argv) {
    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --shots <shots-file>              shot segmentation (exclusive with --annotations)\n");
    options.AddUsage("  --annotations <annotation-file>   shot reference annotations (exclusive with --shots)\n");
    // TODO i was here
    //options.AddUsage("  --multi <frames-per-shot>   shot reference annotations (exclusive with --shots)\n");

    std::string shotFile = options.Get<std::string>("--shots", "");
    std::string annotationFile = options.Get<std::string>("--annotations", "");

    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(options.Size() != 0 || (annotationFile == "" && shotFile == "") || (annotationFile != "" && shotFile != "")) options.Usage();

    std::map<int, amu::ShotType> shotTypes;
    if(annotationFile != "") {
        shotTypes = amu::ShotType::Read(annotationFile, video.GetShowName());
    } else if(shotFile != "") {
        std::vector<amu::ShotSegment> shots = amu::ShotSegment::Read(shotFile);
        for(size_t i = 0; i < shots.size(); i++) {
            shotTypes[shots[i].frame] = amu::ShotType(video.GetShowName(), shots[i].frame, amu::ShotLabel_Set);
        }
    }

    amu::FeatureExtractor extractor;
    amu::LibLinearClassifier classifier("model");

    cv::Mat image;
    for(std::map<int, amu::ShotType>::const_iterator shot = shotTypes.begin(); shot != shotTypes.end(); shot++) {
        video.Seek(shot->first);
        if(!video.ReadFrame(image) || image.empty()) {
            std::cerr << "ERROR: reading frame " << video.GetIndex() << "\n";
            continue;
        }
        std::vector<float> features = extractor.Compute(image);
        std::cerr << shot->second.label << " " << classifier.Classify(features) << "\n";

        std::cout << shot->second.label;
        for(size_t i = 0; i < features.size(); i++) {
            std::cout << " " << i + 1 << ":" << features[i];
        }
        std::cout << "\n";
    }
    return 0;
}
