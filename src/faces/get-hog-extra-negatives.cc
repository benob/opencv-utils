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
            //std::cout << value << "\n";
        }
    }
    return true;
}

struct Person {
    std::string name;
    int keyFrame;
    int startFrame;
    int endFrame;
    cv::Rect rect;
    std::vector<cv::Point> polygon;
};

// warning: assumes valid xgtf!!!
void ReadXgtf(const char* filename, std::vector<Person>& output) {
    amu::Node* root = amu::ParseXML(filename);
    if(root == NULL) return;
    amu::Node* sourceFile = root->Find("sourcefile");
    int hSize = amu::ParseInt(sourceFile->Find("attribute", "name", "H-FRAME-SIZE")->children[0]->attributes["value"]);
    int vSize = amu::ParseInt(sourceFile->Find("attribute", "name", "V-FRAME-SIZE")->children[0]->attributes["value"]);
    std::vector<amu::Node*> found;
    root->Find("object", found, "name", "PERSONNE");
    for(size_t i = 0; i < found.size(); i++) {
        Person person;
        amu::Node* object = found[i];
        person.name = object->Find("attribute", "name", "NOM")->children[0]->attributes["value"];
        person.startFrame = amu::ParseInt(object->Find("attribute", "name", "STARTFRAME")->children[0]->attributes["value"]);
        person.endFrame = amu::ParseInt(object->Find("attribute", "name", "ENDFRAME")->children[0]->attributes["value"]);
        person.keyFrame = amu::ParseInt(object->attributes["framespan"]);
        std::vector<amu::Node*> points;
        object->Find("data:point", points);
        for(size_t j = 0; j < points.size(); j++) {
            int x = amu::ParseInt(points[j]->attributes["x"]) * 1024 / hSize;
            int y = amu::ParseInt(points[j]->attributes["y"]) * 576 / vSize;
            if(person.polygon.size() == 0 || person.rect.x > x) person.rect.x = x; // compute bounding box
            if(person.polygon.size() == 0 || person.rect.y > y) person.rect.y = y;
            if(person.polygon.size() == 0 || person.rect.width < x) person.rect.width = x;
            if(person.polygon.size() == 0 || person.rect.height < y) person.rect.height = y;
            person.polygon.push_back(cv::Point(x, y));
        }
        person.rect.width -= person.rect.x;
        person.rect.height -= person.rect.y;
        output.push_back(person);
    }
    delete root;
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
    options.AddUsage("  --xgtf <reference-xgtf>           xgtf file containing face polygons\n");
    options.AddUsage("  --window <size>                   size of hog classifier (default=32)\n");
    options.AddUsage("  --grow <ratio>                    grow reference by ratio (default=1.5)\n");
    options.AddUsage("  --negatives <num>                 number of random negatives per key frame (default=20)\n");
    options.AddUsage("  --model <model-file>              add negatives from wrong detections by previous model \n");
    std::string xgtfFilename = options.Get<std::string>("--xgtf", "");
    int window = options.Get("--window", 32);
    double grow = options.Get("--grow", 1.5);
    int negatives = options.Get("--negatives", 20);
    std::string hogModelFile = options.Get<std::string>("--model", "");
    double scale = options.Read("--scale", 1.0);

    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(options.Size() != 0 || xgtfFilename == "") options.Usage();

    std::vector<Person> persons;
    ReadXgtf(xgtfFilename.c_str(), persons);

    cv::HOGDescriptor hog(cv::Size(window, window), cv::Size(16,16), cv::Size(8,8), cv::Size(8,8), 9);

    if(hogModelFile != "") {
        std::vector<float> model;
        ReadLibLinearModel(hogModelFile, model);
        hog.setSVMDetector(model);
    }

    std::map<int, std::vector<int> > frames;
    for(size_t p = 0; p < persons.size(); p++) {
        frames[persons[p].keyFrame].push_back(p);
    }

    cv::Mat image;
    cv::Size size = video.GetSize();
    for(std::map<int, std::vector<int> >::iterator frame = frames.begin(); frame != frames.end(); frame++) {
        video.Seek(frame->first);
        video.ReadFrame(image);
        for(size_t p = 0; p < frame->second.size(); p++) {
            Person& person = persons[frame->second[p]];
            //std::cerr << person.name << "\n";
            amu::Scale(person.rect, scale);

            cv::Rect centered;
            if(person.rect.width < person.rect.height) {
                centered.x = person.rect.x;
                centered.y = person.rect.y + person.rect.height / 2 - person.rect.width / 2;
                centered.width = person.rect.width;
                centered.height = person.rect.width;
            } else {
                centered.x = person.rect.x + person.rect.width / 2 - person.rect.height / 2;
                centered.y = person.rect.y;
                centered.width = person.rect.height;
                centered.height = person.rect.height;
            }
            amu::Shrink(centered, grow * 0.8);
            person.rect = centered;
            //amu::Shrink(person.rect, 1.5);
            person.rect = centered;
            for(int i = 0; i < 4; i++) {
                int border = centered.width;
                cv::Mat bordered;
                cv::copyMakeBorder(image, bordered, border, border, border, border, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
                cv::Mat extended(bordered, centered + cv::Point(border, border));

                /*int border = (centered.width * (grow - 1)) / 2;
                cv::Mat face(image, centered);
                cv::Mat extended(face.rows * grow, face.cols * grow, face.depth());
                cv::copyMakeBorder(face, extended, border, border, border, border, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));*/
                //cv::imshow("face", extended);
                //cv::waitKey(0);
                if(extended.empty()) continue;
                cv::resize(extended, extended, cv::Size(window, window));

                //cv::rectangle(image, person.rect, cv::Scalar(255, 0, 0), 1); // bleu
                //cv::rectangle(image, extended, cv::Scalar(0, 255, 0), 1);
                PrintDescriptors(hog, extended, "+1");
                cv::flip(extended, extended, 1);
                PrintDescriptors(hog, extended, "+1");
                amu::Shrink(centered, 1.1);
            }

        }
        int smallestFace = -1;
        for(size_t p = 0; p < frame->second.size(); p++) {
            Person& person = persons[frame->second[p]];
            if(smallestFace == -1 || person.rect.area() < smallestFace) {
                smallestFace = person.rect.area();
            }
        }
        int numNegative = 0;
        int numTries = 0;
        do {
            cv::Rect rect;
            rect.x = rand() % (size.width - 16);
            rect.y = rand() % (size.height - 16);
            //rect.width = rand() % (1024 - rect.x - 16) + 16;
            //rect.height = rand() % (576 - rect.y - 16) + 16;
            rect.width = rand() % (size.width - rect.x - 16) + 16;
            if(rect.width + rect.y > size.height) continue;
            rect.height = rect.width;
            //if(rect.area() >= smallestFace) {
                bool overlap = false;
                for(size_t p = 0; p < frame->second.size(); p++) {
                    if((persons[frame->second[p]].rect & rect).area() > persons[frame->second[p]].rect.area() / 2) {
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
            //}
            numTries++;
        } while(numNegative < negatives && numTries < negatives * 4);

        if(hogModelFile != "") {
            std::vector<cv::Rect> found;
            int groupThreshold = 2;
            cv::Size padding(cv::Size(0, 0));
            cv::Size winStride(cv::Size(4, 4));
            double hitThreshold = 0; // tolerance
            hog.detectMultiScale(image, found, hitThreshold, winStride, padding, 1.1, groupThreshold);
            for(size_t i = 0; i < found.size(); i++) {
                bool correct = false;
                for(size_t p = 0; p < frame->second.size(); p++) {
                    if((persons[frame->second[p]].rect & found[i]).area() > persons[frame->second[p]].rect.area() / 2
                            && (persons[frame->second[p]].rect & found[i]).area() > found[i].area() / 2) {
                        correct = true;
                        break;
                    }
                }
                if(!correct) {
                    cv::Mat negative(image, found[i]);
                    cv::Mat resized;
                    cv::resize(negative, resized, cv::Size(window, window));
                    PrintDescriptors(hog, resized, "-1");
                    //cv::rectangle(image, found[i], cv::Scalar(0, 0, 255), 1);
                } else {
                    //cv::rectangle(image, found[i], cv::Scalar(0, 255, 0), 1);
                }
            }
            //cv::imshow("image", image);
            //cv::waitKey(0);
        }

    }
}
