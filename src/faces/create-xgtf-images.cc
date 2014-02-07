#include <map>

#include "repere.h"
#include "xml.h"
#include "face.h"

struct Person {
    std::string name;
    int keyFrame;
    int startFrame;
    int endFrame;
    cv::Rect rect;
    std::vector<cv::Point> polygon;
};

// warning: assumes valid xgtf!!!
std::map<int, std::vector<Person> > ReadXgtf(const char* filename) {
    std::map<int, std::vector<Person> > output;
    amu::Node* root = amu::ParseXML(filename);
    if(root == NULL) return output;
    amu::Node* sourceFile = root->Find("sourcefile");
    int hSize = amu::ParseInt(sourceFile->Find("attribute", "name", "H-FRAME-SIZE")->children[0]->attributes["value"]);
    int vSize = amu::ParseInt(sourceFile->Find("attribute", "name", "V-FRAME-SIZE")->children[0]->attributes["value"]);
    std::vector<amu::Node*> found;
    root->Find("object", found, "framespan");
    for(size_t i = 0; i < found.size(); i++) {
        amu::Node* object = found[i];
        output[amu::ParseInt(object->attributes["framespan"])] =  std::vector<Person>();
    }
    found.clear();
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
        output[person.keyFrame].push_back(person);
    }
    delete root;
    return output;
}

int main(int argc, char** argv) {
    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --xgtf <reference-xgtf>           xgtf file containing face polygons\n");
    options.AddUsage("  --output-dir <dir>                directory where images are output\n");
    std::string xgtfFilename = options.Get<std::string>("--xgtf", "");
    std::string outputDir = options.Get<std::string>("--output-dir", "");
    double scale = options.Read<double>("--scale", 1.0);

    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(options.Size() != 0 || xgtfFilename == "" || outputDir == "") options.Usage();

    std::map<int, std::vector<Person> > persons = ReadXgtf(xgtfFilename.c_str());

    cv::Mat image;

    for(std::map<int, std::vector<Person> >::iterator frame = persons.begin(); frame != persons.end(); frame++) {
        video.Seek(frame->first);
        video.ReadFrame(image);
        for(std::vector<Person>::iterator person = frame->second.begin(); person != frame->second.end(); person++) {
            amu::Scale(person->rect, scale);

            for(size_t i = 0; i < person->polygon.size(); i++) {
                cv::line(image, person->polygon[i] * scale, person->polygon[(i + 1) % person->polygon.size()] * scale, cv::Scalar(0, 255, 0));
            }
            //cv::rectangle(image, person->rect, cv::Scalar(255, 0, 0), 1);
            cv::putText(image, person->name.c_str(), cv::Point(person->rect.x + 2, person->rect.y + 11), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255));
        }
        std::stringstream name;
        name << outputDir << "/" << frame->first << ".png";
        std::cout << name.str() << "\n";
        if(!cv::imwrite(name.str(), image)) {
            std::cerr << "ERROR: failed to write image file\n";
            return 1;
        }
        //cv::imshow("image", image);
        //cv::waitKey(0);
    }
    return 0;
}
