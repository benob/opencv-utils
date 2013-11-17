#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "repere.h"

int main(int argc, char** argv) {
    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --templates <templates>               specify template definition file\n");
    options.AddUsage("  --show                                show the extracted subshots\n");
    options.AddUsage("  --shots <shots-file>                  read frames from shot detector output instead of stdin\n");
    options.AddUsage("  --splits <split-file>                 types of split for each frame\n");

    std::string templateFile = options.Get<std::string>("--templates", "");
    std::string shotsFile = options.Get<std::string>("--shots", "");
    std::string splitFile = options.Get<std::string>("--splits", "");
    bool show = options.IsSet("--show");
    double scale = options.Read("--scale", 1.0);

    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(options.Size() != 0 || templateFile == "" || splitFile == "") options.Usage();

    cv::HOGDescriptor hog(cv::Size(128, 64), cv::Size(16,16), cv::Size(8,8), cv::Size(8,8), 9);

    std::fstream templateStream(templateFile.c_str());
    if(!templateStream) {
        std::cerr << "ERROR: reading templates from \"" << templateFile << "\"\n";
        return 1;
    }
    std::vector<amu::Split> templates;
    amu::Split::ParseTemplates(templateStream, templates, scale);
    
    std::vector<std::string> splitTypes;
    std::ifstream splitStream(splitFile.c_str());
    std::string line;
    while(std::getline(splitStream, line)) {
        std::stringstream reader(line);
        int frame; std::string type;
        reader >> frame >> type;
        splitTypes.push_back(type);
    }

    std::vector<amu::ShotSegment> shots;
    if(shotsFile != "") {
        if(amu::ShotSegment::ParseShotSegmentation(shotsFile, shots) == false) return 1;
    } else {
        for(int i = 0; i < video.NumFrames(); i++) shots.push_back(amu::ShotSegment(i, i, i, i, i, i, i));
    }

    for(size_t shot = 0; shot < shots.size(); shot++) {
    /*std::string line;
    while(std::getline(std::cin, line)) {
        std::stringstream reader(line);
        std::string filename, type;
        reader >> filename >> type;
        std::string name(filename, filename.rfind('/') + 1);
        cv::Mat image = cv::imread(filename, 1);*/
        cv::Mat image;
        video.Seek(shots[shot].frame);
        video.ReadFrame(image);
        std::string name = amu::BaseName(video.GetFrameName());
        if(image.rows == 0) {
            //std::cerr << "ERROR: reading \"" << filename << "\"\n";
            return 1;
        } else {
            cv::Mat gray;
            cv::cvtColor(image, gray, CV_BGR2GRAY);
            cv::resize(gray, gray, cv::Size(128, 64));

            std::vector<float> descriptorsValues;
            std::vector<cv::Point> locations;
            hog.compute(gray, descriptorsValues, cv::Size(0,0), cv::Size(0,0), locations);

            std::cout << name;
            for(size_t i = 0; i < descriptorsValues.size(); i++) {
                std::cout << " " << descriptorsValues[i];
            }
            std::cout << "\n";

            //cv::resize(image, image, cv::Size(1024, 576));

            int argmax = amu::TemplateMatcher::Match(image, templates, splitTypes[shot]);
            if(argmax != -1) {
                for(int i = 0; i < templates[argmax].subshots.size(); i++) {
                    const amu::SubShot subshot = templates[argmax].subshots[i];
                    cv::Mat subImage(image, cv::Rect(subshot.x, subshot.y, subshot.width, subshot.height));
                    cv::Mat gray;
                    cv::cvtColor(subImage, gray, CV_BGR2GRAY);
                    cv::resize(gray, gray, cv::Size(128, 64));
                    if(show) {
                        std::stringstream windowName; windowName << i;
                        cv::imshow(windowName.str(), gray);
                    }

                    std::vector<float> descriptorsValues;
                    std::vector<cv::Point> locations;
                    hog.compute(gray, descriptorsValues, cv::Size(0,0), cv::Size(0,0), locations);

                    std::cout << name << ":" << i;
                    for(size_t i = 0; i < descriptorsValues.size(); i++) {
                        std::cout << " " << descriptorsValues[i];
                    }
                    std::cout << "\n";
                }
                if(show) cv::waitKey(0);
            }
        }
    }
    return 0;
}
