#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "repere.h"

namespace amu {

    class LinearClassifier {
        private:
            bool loaded;
            std::vector<std::string> labels;
            std::vector<std::vector<double> > weights;

        public:
            LinearClassifier(const std::string& filename = "") : loaded(false) { 
                if(filename != "") Load(filename);
            }

            /* model format: labels on 1st line, then weight per label of each feature on following lines */
            bool Load(const std::string& filename) {
                loaded = false;
                std::fstream input(filename.c_str());
                if(!input) {
                    std::cerr << "ERROR: could not load hog model \"" << filename << "\"\n";
                    return false;
                }
                std::string line, label;
                if(!std::getline(input, line)) {
                    std::cerr << "ERROR: did not find list of labels in \"" << filename << "\"\n";
                    return false;
                }
                std::stringstream reader(line);
                while(std::getline(reader, label, ' ')) {
                    labels.push_back(label);
                    weights.push_back(std::vector<double>());
                }
                while(std::getline(input, line)) {
                    std::stringstream reader(line);
                    for(size_t label = 0; label < labels.size(); label++) { 
                        double weight;
                        reader >> weight;
                        weights[label].push_back(weight);
                    }
                }
                //std::cout << labels.size() << "\n";
                loaded = true;
                return true;
            }

            void ComputeScores(const std::vector<float>& descriptors, std::vector<double>& scores) const {
                scores.clear();
                scores.resize(labels.size(), 0.0);
                for(size_t label = 0; label < labels.size(); label++) {
                    for(size_t descriptor = 0; descriptor < weights[label].size(); descriptor++) {
                        scores[label] += weights[label][descriptor] * descriptors[descriptor];
                    }
                }
            }

            std::string Classify(const std::vector<float>& descriptors) const {
                double max = 0;
                int argmax = -1;
                std::vector<double> scores;
                ComputeScores(descriptors, scores);
                for(size_t label = 0; label < labels.size(); label++) {
                    if(argmax == -1 || scores[label] > max) {
                        max = scores[label];
                        argmax = label;
                    }
                }
                return labels[argmax];
            }
    };
}

/* hack to add features for show and split TODO: cleanup */
void AddFeatures(const std::string& split, int subshot, const std::string& showname, const std::vector<float>& hogDescriptors, std::vector<float>& output) {
    const char* shows[9] = {"BFMTV_BFMStory", "BFMTV_CultureEtVous", "BFMTV_PlaneteShowbiz", "LCP_CaVousRegarde", 
        "LCP_EntreLesLignes", "LCP_LCPInfo13h30", "LCP_LCPInfo20h30", "LCP_PileEtFace", "LCP_TopQuestions"}; 
    const char* splits[31] = {"1-full", "2-vertical:0", "2-vertical:1", "2-big-left:0", "2-big-left:1", "3-big-left:0", 
        "3-big-left:1", "3-big-left:2", "3-even:0", "3-even:1", "3-even:2", "4-even:0", "4-even:1", "4-even:2", 
        "4-even:3", "1-other", "4-big-left:0", "4-big-left:1", "4-big-left:2", "4-big-left:3", "4-big-right:0", 
        "4-big-right:1", "4-big-right:2", "4-big-right:3", "3-big-right:0", "3-big-right:1", "3-big-right:2", 
        "2-big-right:0", "2-big-right:1", "2-horizontal:0", "2-horizontal:1"};
    output.clear();
    output.resize(1000, 0);
    std::string showFeature = showname.substr(0, showname.find('_', showname.find('_') + 1));
    bool found = false;
    for(size_t i = 0; i < 9; i++) {
        if(shows[i] == showFeature) {
            output[i] = 1;
            found = true;
        }
    }
    if(!found) {
        std::cerr << "WARNING: show feature \"" << showFeature << "\" not found\n";
    }
    std::stringstream feature;
    if(subshot != -1) feature << split << ":" << subshot;
    else feature << split;
    std::string splitFeature = feature.str();
    found = false;
    for(size_t i = 0; i < 31; i++) {
        if(splits[i] == splitFeature) {
            output[i + 99] = 1;
            found = true;
        }
    }
    if(!found) {
        std::cerr << "WARNING: split feature \"" << splitFeature << "\" not found\n";
    }
    for(size_t i = 0; i < hogDescriptors.size(); i++) {
        output.push_back(hogDescriptors[i]);
    }
}

int main(int argc, char** argv) {
    amu::CommandLine options(argv, "[options] <subshot-model+>\n");
    options.AddUsage("  --templates <templates>               specify template definition file\n");
    options.AddUsage("  --show                                show the extracted subshots\n");
    options.AddUsage("  --shots <shots-file>                  read frames from shot detector output instead of stdin\n");
    options.AddUsage("  --split-model <model>                 liblinear model for detecting split type\n");

    std::string templateFile = options.Get<std::string>("--templates", "");
    std::string shotsFile = options.Get<std::string>("--shots", "");
    std::string splitModelFile = options.Get<std::string>("--split-model", "");
    bool show = options.IsSet("--show");
    double scale = options.Read("--scale", 1.0);

    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(templateFile == "" || splitModelFile == "") options.Usage();

    cv::HOGDescriptor hog(cv::Size(128, 64), cv::Size(16,16), cv::Size(8,8), cv::Size(8,8), 9);

    std::vector<amu::Split> templates;
    amu::Split::ParseTemplates(templateFile, templates, scale);

    std::vector<amu::ShotSegment> shots;
    amu::ShotSegment::ParseShotSegmentation(shotsFile, shots);

    amu::LinearClassifier splitClassifier(splitModelFile);
    std::vector<amu::LinearClassifier> models;
    for(size_t i = 0; i < options.Size(); i++) {
        models.push_back(amu::LinearClassifier(options[i]));
    }

    cv::Mat image, gray;
    for(size_t shot = 0; shot < shots.size(); shot++) {
        video.Seek(shots[shot].frame);
        video.ReadFrame(image);
        //std::string name = amu::BaseName(video.GetFrameName());

        cv::Mat gray;
        cv::cvtColor(image, gray, CV_BGR2GRAY);
        cv::resize(gray, gray, cv::Size(128, 64));
        std::vector<float> descriptorsValues;
        std::vector<cv::Point> locations;
        hog.compute(gray, descriptorsValues, cv::Size(0,0), cv::Size(0,0), locations);

        std::string splitType = splitClassifier.Classify(descriptorsValues);

        int argmax = amu::TemplateMatcher::Match(image, templates, splitType);
        if(argmax != -1) {
            std::cout << shots[shot].frame << " " << -1 << " " << splitType << "\n";
            for(int i = 0; i < templates[argmax].subshots.size(); i++) {
                const amu::SubShot subshot = templates[argmax].subshots[i];
                cv::Mat subImage(image, cv::Rect(subshot.x, subshot.y, subshot.width, subshot.height));
                cv::Mat gray;
                cv::cvtColor(subImage, gray, CV_BGR2GRAY);
                cv::resize(gray, gray, cv::Size(128, 64));
                if(show) {
                    std::stringstream windowName; windowName << i;
                    cv::imshow(windowName.str(), subImage);
                }

                std::vector<float> descriptorsValues;
                std::vector<cv::Point> locations;
                hog.compute(gray, descriptorsValues, cv::Size(0,0), cv::Size(0,0), locations);

                std::cout << shots[shot].frame << " " << i << " subshot";
                std::vector<float> features;
                AddFeatures(splitType, i, video.GetShowName(), descriptorsValues, features);
                for(size_t model = 0; model < models.size(); model++) {
                    std::cout << " " << models[model].Classify(features);
                }

                std::cout << "\n";
            }
        } else {
            std::cout << shots[shot].frame << " " << -1 << " " << splitType;
            std::vector<float> features;
            AddFeatures(splitType, -1, video.GetShowName(), descriptorsValues, features);
            for(size_t model = 0; model < models.size(); model++) {
                std::cout << " " << models[model].Classify(features);
            }
            std::cout << "\n";
        }

        if(show) {
            cv::imshow("image", image);
            cv::waitKey(0);
        }
    }
    return 0;
}
