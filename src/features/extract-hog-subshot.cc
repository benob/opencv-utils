#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "repere.h"

int main(int argc, char** argv) {
    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --templates <templates>               specify template definition file\n");

    std::string templateFile = options.Get<std::string>("--templates", "");
    if(options.Size() != 0 || templateFile == "") options.Usage();

    cv::HOGDescriptor hog(cv::Size(128, 64), cv::Size(16,16), cv::Size(8,8), cv::Size(8,8), 9);

    std::fstream templateStream(templateFile.c_str());
    if(!templateStream) {
        std::cerr << "ERROR: reading templates from \"" << templateFile << "\"\n";
        return 1;
    }
    std::vector<amu::Split> templates;
    amu::Split::ParseTemplates(templateStream, templates, 1);
    
    std::string line;
    while(std::getline(std::cin, line)) {
        std::stringstream reader(line);
        std::string filename, type;
        reader >> filename >> type;
        std::string name(filename, filename.rfind('/') + 1);
        cv::Mat image = cv::imread(filename, 1);
        if(image.rows == 0) {
            std::cerr << "ERROR: reading \"" << filename << "\"\n";
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

            cv::resize(image, image, cv::Size(1024, 576));

            int argmax = amu::TemplateMatcher::Match(image, templates, type);
            if(argmax != -1) {
                for(int i = 0; i < templates[argmax].subshots.size(); i++) {
                    const amu::SubShot subshot = templates[argmax].subshots[i];
                    cv::Mat subImage(image, cv::Rect(subshot.x, subshot.y, subshot.width, subshot.height));
                    cv::Mat gray;
                    cv::cvtColor(subImage, gray, CV_BGR2GRAY);
                    cv::resize(gray, gray, cv::Size(128, 64));

                    std::vector<float> descriptorsValues;
                    std::vector<cv::Point> locations;
                    hog.compute(gray, descriptorsValues, cv::Size(0,0), cv::Size(0,0), locations);

                    std::cout << name << ":" << i;
                    for(size_t i = 0; i < descriptorsValues.size(); i++) {
                        std::cout << " " << descriptorsValues[i];
                    }
                    std::cout << "\n";
                }
            }
        }
    }
    return 0;
}
