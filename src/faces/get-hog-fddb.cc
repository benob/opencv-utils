#include <map>

#include "repere.h"
#include "xml.h"
#include "face.h"

bool ReadLibLinearModel(const std::string& filename, std::vector<float>& model) {
    std::fstream modelStream(filename.c_str());
    if(!modelStream) {
        std::cerr << "ERROR: could not load hog model \"" << filename << "\"\n";
        return false;
    }
    model.clear();
    std::string line;
    bool keepWeights = false;
    while(std::getline(modelStream, line)) {
        if(line == "w") keepWeights = true;
        else if(keepWeights) {
            std::stringstream reader(line);
            float value;
            reader >> value;
            model.push_back(value);
            std::cout << value << "\n";
        }
    }
    return true;
}

void PrintDescriptors(const cv::HOGDescriptor& hog, const cv::Mat& image, const std::string& label) {
    std::vector<float> descriptorsValues;
    std::vector<cv::Point> locations;
    hog.compute(image, descriptorsValues, cv::Size(0,0), cv::Size(0,0), locations);

    std::cout << label;
    for(size_t i = 0; i < descriptorsValues.size(); i++) {
        std::cout << " " << i + 1 << ":" << descriptorsValues[i];
    }
    std::cout << "\n";
}

int main(int argc, char** argv) {
    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --window <size>                   size of hog classifier (default=32)\n");
    options.AddUsage("  --grow <ratio>                    grow reference by ratio (default=1.5)\n");
    options.AddUsage("  --negatives <num>                 number of random negatives per key frame (default=20)\n");
    options.AddUsage("  --model <model-file>              add negatives from wrong detections by previous model \n");
    options.AddUsage("  --show                            show detections and reference\n");
    int window = options.Get("--window", 32);
    double grow = options.Get("--grow", 1.5);
    int negatives = options.Get("--negatives", 20);
    bool show = options.IsSet("--show");
    std::string hogModelFile = options.Get<std::string>("--model", "");

    if(options.Size() != 0) options.Usage();

    cv::HOGDescriptor hog(cv::Size(window, window), cv::Size(16,16), cv::Size(8,8), cv::Size(8,8), 9);

    if(hogModelFile != "") {
        std::vector<float> model;
        ReadLibLinearModel(hogModelFile, model);
        hog.setSVMDetector(model);
    }

    cv::Mat image;
    std::string line;

    while(std::getline(std::cin, line)) {
        std::string filename;
        int numFaces;
        std::vector<cv::Rect> faces;
        std::stringstream reader(line);
        reader >> filename >> numFaces;
        std::cerr << filename << " " << numFaces << "\n";
        for(int i = 0; i < numFaces; i++) {
            cv::Rect rect;
            reader >> rect.x >> rect.y >> rect.width >> rect.height;
            faces.push_back(rect);
        }

        image = cv::imread(filename);
        cv::Size size = image.size();

        for(int p = 0; p < numFaces; p++) {
            cv::Rect& person = faces[p];

            cv::Rect centered;
            if(person.width < person.height) {
                centered.x = person.x;
                centered.y = person.y + person.height / 2 - person.width / 2;
                centered.width = person.width;
                centered.height = person.width;
            } else {
                centered.x = person.x + person.width / 2 - person.height / 2;
                centered.y = person.y;
                centered.width = person.height;
                centered.height = person.height;
            }
            int border = centered.width;
            cv::Mat bordered;
            cv::copyMakeBorder(image, bordered, border, border, border, border, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
            amu::Shrink(centered, grow);
            faces[p] = centered;
            cv::Mat extended(bordered, faces[p] + cv::Point(border, border));
            //cv::imshow("face", extended);
            //cv::waitKey(0);
            cv::resize(extended, extended, cv::Size(window, window));

            //cv::rectangle(image, person, cv::Scalar(255, 0, 0), 1);
            //cv::rectangle(image, extended, cv::Scalar(0, 255, 0), 1);
            PrintDescriptors(hog, extended, "+1");
            cv::flip(extended, extended, 1);
            PrintDescriptors(hog, extended, "+1");

        }
        int numNegative = 0;
        do {
            cv::Rect rect;
            rect.x = rand() % (size.width - 16);
            rect.y = rand() % (size.height - 16);
            rect.width = rand() % (size.width - rect.x - 16) + 16;
            if(rect.width + rect.y > size.height) continue;
            rect.height = rect.width;

            bool overlap = false;
            for(size_t p = 0; p < numFaces; p++) {
                if((faces[p] & rect).area() > faces[p].area() / 2) {
                    overlap = true;
                    break;
                }
            }
            if(!overlap) {
                cv::Mat negative(image, rect);
                cv::Mat resized;
                cv::resize(negative, resized, cv::Size(window, window));
                PrintDescriptors(hog, resized, "-1");
                numNegative++;
                //cv::rectangle(image, rect, cv::Scalar(0, 0, 255), 1);
            }
        } while(numNegative < negatives);

        if(hogModelFile != "") {
            std::vector<cv::Rect> found;
            int groupThreshold = 2;
            cv::Size padding(cv::Size(0, 0));
            cv::Size winStride(cv::Size(4, 4));
            double hitThreshold = 1; // tolerance
            hog.detectMultiScale(image, found, hitThreshold, winStride, padding, 1.1, groupThreshold);
            for(size_t i = 0; i < found.size(); i++) {
                bool correct = false;
                for(size_t p = 0; p < numFaces; p++) {
                    if((faces[p] & found[i]).area() > faces[p].area() / 2
                            && (faces[p] & found[i]).area() > found[i].area() / 2) {
                        correct = true;
                        break;
                    }
                }
                if(!correct) {
                    cv::Mat negative(image, found[i]);
                    cv::Mat resized;
                    cv::resize(negative, resized, cv::Size(window, window));
                    PrintDescriptors(hog, resized, "-1");
                    if(show) cv::rectangle(image, found[i], cv::Scalar(0, 0, 255), 1);
                } else {
                    if(show) cv::rectangle(image, found[i], cv::Scalar(0, 255, 0), 1);
                }
            }
            for(int p = 0; p < numFaces; p++) {
                cv::rectangle(image, faces[p], cv::Scalar(255, 255, 0), 1);
            }
            if(show) {
                cv::imshow("image", image);
                cv::waitKey(0);
            }
        }

    }
}
