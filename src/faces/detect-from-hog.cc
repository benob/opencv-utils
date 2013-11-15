
#include "repere.h"

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

/*bool ReadSvmLightModel(const std::string& filename, std::vector<float>& model) {
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
}*/

int main(int argc, char** argv) {
    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --hog-model <model>               model as a text file containing a vector of weights\n");
    options.AddUsage("  --window <size>                   size of hog classifier (default=32)\n");
    options.AddUsage("  --shots <shots>                   shot boundaries\n");
    std::string hogModelFile = options.Get<std::string>("--hog-model", "");
    std::string shotFile = options.Get<std::string>("--shots", "");
    int window = options.Get("--window", 32);
    double scale = options.Read("--scale", 1.0);

    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(options.Size() != 0 || hogModelFile == "" || shotFile == "") options.Usage();

    cv::HOGDescriptor hog(cv::Size(window, window), cv::Size(16,16), cv::Size(8,8), cv::Size(8,8), 9);

    std::vector<float> model;
    ReadLibLinearModel(hogModelFile, model);
    hog.setSVMDetector(model);

    std::vector<amu::ShotSegment> shots;
    amu::ShotSegment::ParseShotSegmentation(shotFile, shots);

    cv::Mat image, gray;
    cv::Size size = video.GetSize();
    for(size_t shot = 50; shot < shots.size(); shot++) {
        video.Seek(shots[shot].frame);
        video.ReadFrame(image);
        cv::cvtColor(image, gray, CV_BGR2GRAY);
        std::vector<cv::Rect> found;
        int groupThreshold = 2;
        cv::Size padding(cv::Size(0, 0));
        cv::Size winStride(cv::Size(4, 4));
        double hitThreshold = 2.5; // tolerance
        hog.detectMultiScale(gray, found, hitThreshold, winStride, padding, 1.05, groupThreshold);
        for(size_t i = 0; i < found.size(); i++) {
            cv::rectangle(image, found[i], cv::Scalar(64, 255, 64), 1);
        }
        cv::imshow("image", image);
        cv::waitKey(0);
    }
}
