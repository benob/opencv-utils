#include <opencv2/opencv.hpp>

#include "repere.h"
#include "xml.h"
#include "face.h"
#include "bow.h"
#include <map>

#define foreach( i, c )\
    typedef __typeof__( c ) c##_CONTAINERTYPE;\
    for( c##_CONTAINERTYPE::iterator i = c.begin(); i != c.end(); ++i )

struct Person {
    std::string name;
    std::string role;
    int keyFrame;
    int startFrame;
    int endFrame;
    cv::Rect rect;
    std::vector<cv::Point> polygon;
};

struct PersonLess {
    bool operator()(const Person&a, const Person&b) {
        if (a.keyFrame == b.keyFrame) {
            if(a.rect == b.rect) {
                return a.name < b.name;
            } else {
                return a.rect.x < b.rect.x;
            }
        } else {
            return (a.keyFrame < b.keyFrame);
        }
    }
};

// warning: assumes valid xgtf!!!
void ReadXgtf(const char* filename, std::map<int, std::vector<Person> > &output) {
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
        if(object->Find("attribute", "name", "ROLE")->children.size() == 0) {
            person.role = "R5";
        } else {
            person.role = object->Find("attribute", "name", "ROLE")->children[0]->attributes["value"];
        }
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
        output[person.keyFrame].push_back(person);
    }
    delete root;
}

int main(int argc, char** argv) {
    if(argc != 3) {
        std::cerr << "usage: " << argv[0] << " <video>" << std::endl;
        return 1;
    }

    //amu::VideoReader video(argv[1]);
    cv::VideoCapture video(argv[1]);
    if(!video.isOpened()) {
        std::cerr << "error opening video " << argv[1] << "\n";
        return 1;
    }
    amu::Idx idx(argv[2]);

    cv::Mat image, original, gray, oldGray, skin;
    std::string line;
    std::map<int, std::vector<Person> > persons_by_frame;
    ReadXgtf("/dev/stdin", persons_by_frame);

    std::map<int, std::string> frameLabel;
    std::map<std::string, std::map<std::string, int> > personId;

    amu::BOWFeatures features("bow-vocabulary.keyframes-png-v6-train.1000");
    if(!features.IsLoaded()) {
        return 1;
    }

    foreach(frame_iterator, persons_by_frame) {
        int keyFrame = frame_iterator->first;
        std::vector<Person>& persons = frame_iterator->second;
        std::sort(persons.begin(), persons.end(), PersonLess());
        std::stringstream id;
        bool has_R5 = false;
        for(size_t p = 0; p < persons.size(); p++) {
            std::string name = persons[p].name;
            if(name.substr(0, 7) == "Inconnu") continue;
            std::string role = persons[p].role;
            if(role == "R5") {
                has_R5 = true;
                continue;
            }
            if(personId[role].find(name) == personId[role].end()) {
                personId[role][name] = personId[role].size();
            }
            id << "+";
            id << role ;//<< ":" << personId[role][name];
        }
        if(has_R5) id << "+R5";
        frameLabel[keyFrame] = id.str();
    }

    foreach(frame_iterator, persons_by_frame) {
        int keyFrame = frame_iterator->first;
        std::vector<Person>& persons = frame_iterator->second;
        //std::cout << keyFrame << " " << frameLabel[keyFrame] << "\n";

        //video.SeekTime(idx.GetTime(person.keyFrame));
        //video.ReadFrame(image, cv::Size(1024 / 2, 576 / 2));
        video.set(CV_CAP_PROP_POS_MSEC, idx.GetTime(keyFrame) * 1000);
        video >> original;
        cv::resize(original, image, cv::Size(1024 / 2, 576 / 2));

        cv::Mat descriptors;
        features.Extract(image, descriptors);

        std::cout << frameLabel[keyFrame];
        for(int i = 0; i < descriptors.cols; i++) {
            float value = descriptors.at<float>(0, i);
            if(value > 1e-5 || value < -1e-5) std::cout << " " << i + 1 << ":" << value;
        }
        std::cout << "\n";

        /*for(size_t p = 0; p < persons.size(); p++) {
            Person& person = persons[p];

            std::vector<cv::Point2f> obj, scene;
            cv::Point2f objMean, sceneMean;
            amu::Tracker tracker;

            std::cout << "    " << person.name << " " << person.role << "\n";
            amu::Scale(person.polygon, 0.5);
            cv::vector<cv::vector<cv::Point> > contours;
            contours.push_back(person.polygon);
            cv::drawContours(image, contours, 0, cv::Scalar(0, 255, 0));

        }
        cv::imshow("display", image);
        cv::waitKey(-1);*/
    }
    return 0;
}
