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

int main(int argc, char** argv) {
    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --xgtf <reference-xgtf>           xgtf file containing face polygons\n");
    options.AddUsage("  --window <size>                   size of hog classifier (default=32)\n");
    options.AddUsage("  --grow <ratio>                    grow reference by ratio (default=1.5)\n");
    options.AddUsage("  --model <model-file>              add negatives from wrong detections by previous model \n");
    options.AddUsage("  --groups <num>                    minimum number of detections that need to be clustered together to fire (default=2)\n");
    options.AddUsage("  --threshold <threshold>           score decision threshold (default=0)\n");
    options.AddUsage("  --show                            show detections\n");
    std::string xgtfFilename = options.Get<std::string>("--xgtf", "");
    int window = options.Get("--window", 32);
    double grow = options.Get("--grow", 1.5);
    std::string hogModelFile = options.Get<std::string>("--model", "");
    double hitThreshold = options.Get("--threshold", 0.0);
    int groupThreshold = options.Get("--groups", 2);
    double scale = options.Read("--scale", 1.0);
    bool show = options.IsSet("--show");

    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(options.Size() != 0 || xgtfFilename == "" || hogModelFile == "") options.Usage();

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

    double numRef = 0, numHyp = 0, numCorrect = 0;

    cv::Mat image;
    cv::Size size = video.GetSize();
    for(std::map<int, std::vector<int> >::iterator frame = frames.begin(); frame != frames.end(); frame++) {
        video.Seek(frame->first);
        video.ReadFrame(image);
        std::cerr << frame->first << "\n";
        for(size_t p = 0; p < frame->second.size(); p++) {
            Person& person = persons[frame->second[p]];
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
            amu::Shrink(centered, grow);
            person.rect = centered;
            if(show) cv::rectangle(image, centered, cv::Scalar(0, 255, 0), 1);
        }
        if(hogModelFile != "") {
            std::vector<cv::Rect> found;
            cv::Size padding(cv::Size(0, 0));
            cv::Size winStride(cv::Size(4, 4));
            hog.detectMultiScale(image, found, hitThreshold, winStride, padding, 1.1, groupThreshold);
            for(size_t i = 0; i < found.size(); i++) {
                //if(found[i].width < 64) continue;
                if(show) cv::rectangle(image, found[i], cv::Scalar(0, 0, 255), 1);
                bool correct = false;
                for(size_t p = 0; p < frame->second.size(); p++) {
                    if((persons[frame->second[p]].rect & found[i]).area() > persons[frame->second[p]].rect.area() / 2
                            && (persons[frame->second[p]].rect & found[i]).area() > found[i].area() / 2) {
                        correct = true;
                        break;
                    }
                }
                if(correct) {
                    numCorrect++;
                }
                numHyp++;
            }
            numRef += frame->second.size();
        }
        if(show) {
            cv::imshow("image", image);
            cv::waitKey(0);
        }

    }
    double recall = numCorrect / numRef;
    double precision = numCorrect / numHyp;
    std::cout << numRef << " " << numHyp << " " << numCorrect << " r=" << recall << " p=" << precision << " f=" << (2 * precision * recall) / (precision + recall) << "\n";
}
