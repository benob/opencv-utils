#include <opencv2/opencv.hpp>

#include "repere.h"
#include "xml.h"
#include "face.h"

#define foreach( i, c )\
    typedef __typeof__( c ) c##_CONTAINERTYPE;\
    for( c##_CONTAINERTYPE::iterator i = c.begin(); i != c.end(); ++i )

struct Person {
    std::string name;
    int keyFrame;
    int startFrame;
    int endFrame;
    cv::Rect rect;
    std::vector<cv::Point> polygon;
};

struct PersonLess {
    bool operator()(const Person&a, const Person&b) {
        return (a.keyFrame < b.keyFrame);
    }
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

    amu::CommandLine options(argv);
    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(options.Size() != 0) options.Usage();

    cv::Mat image, original, gray, oldGray, skin;
    std::string line;
    std::vector<Person> persons;
    ReadXgtf("/dev/stdin", persons);
    std::sort(persons.begin(), persons.end(), PersonLess());
    for(size_t p = 0; p < persons.size(); p++) {
        Person& person = persons[p];

        std::vector<cv::Point2f> obj, scene;
        cv::Point2f objMean, sceneMean;
        amu::Tracker tracker;

        std::cout << person.name << " " << person.keyFrame << "\n";
        amu::Scale(person.polygon, 0.5);
        video.Seek(person.keyFrame);
        video.ReadFrame(image);
        //video.set(CV_CAP_PROP_POS_MSEC, idx.GetTime(person.keyFrame) * 1000);
        //video >> original;
        //cv::resize(original, image, cv::Size(1024 / 2, 576 / 2));
        cv::vector<cv::vector<cv::Point> > contours;
        contours.push_back(person.polygon);
        cv::drawContours(image, contours, 0, cv::Scalar(0, 255, 0));

        cv::imshow("display", image);
        cv::waitKey(-1);
    }
    return 0;
}
