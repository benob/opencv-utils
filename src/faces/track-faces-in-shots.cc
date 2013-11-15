#include <iostream>
#include <list>

#include "repere.h"
#include "commandline.h"


namespace amu {

    double skin(cv::Mat &image) {
        cv::Mat hsv;
        cv::cvtColor(image, hsv, CV_BGR2HSV);
        cv::Mat hue(image.size(), CV_8UC1), saturation(image.size(), CV_8UC1), value(image.size(), CV_8UC1);
        std::vector<cv::Mat> channels;
        channels.push_back(hue);
        channels.push_back(saturation);
        channels.push_back(value);
        cv::split(hsv, channels);
        cv::threshold(hue, hue, 18, 255, CV_THRESH_BINARY_INV);
        cv::threshold(saturation, saturation, 50, 255, CV_THRESH_BINARY);
        cv::threshold(value, value, 80, 255, CV_THRESH_BINARY);
        cv::Mat output;
        cv::bitwise_and(hue, saturation, output);
        cv::bitwise_and(output, value, output);
        cv::Scalar sums = cv::sum(output);
        double result = sums[0] / (output.rows * output.cols * 255);
        return result;
    }

    class Detector {
        private:
            std::vector<cv::CascadeClassifier> detectors;
        public:
            Detector(const std::string& path = ".") {
                detectors.push_back(cv::CascadeClassifier(path + "/haarcascade_frontalface_alt.xml"));
                detectors.push_back(cv::CascadeClassifier(path + "/haarcascade_profileface.xml"));
                /*detectors.push_back(cv::CascadeClassifier(path + "/haarcascade_frontalface_alt2.xml"));
                detectors.push_back(cv::CascadeClassifier(path + "/haarcascade_upperbody.xml"));
                detectors.push_back(cv::CascadeClassifier(path + "/haarcascade_fullbody.xml"));*/
            }

            bool Detect(const cv::Mat& image, double scale = 1.0, int frame = 0, bool show = false) {
                cv::Mat copy;
                if(show) image.copyTo(copy);

                std::vector<cv::Rect> detectedFaces;
                int model = 0;

                for(std::vector<cv::CascadeClassifier>::iterator detector = detectors.begin(); detector != detectors.end(); detector++) {
                    if(detector->empty()) {
                        std::cerr << "ERROR: could not load models\n";
                        return false;
                    }
                    detectedFaces.clear();
                    detector->detectMultiScale(image, detectedFaces, 1.2, 2, 0, cv::Size(), cv::Size());
                    //for(std::vector<cv::Rect>::const_iterator i = detectedFaces.begin(); i != detectedFaces.end(); i++) {
                    for(size_t i = 0; i < detectedFaces.size(); i++) {
                        const cv::Rect& rect = detectedFaces[i];
                        cv::Mat head(image, rect);
                        if(skin(head) < 0.3) continue;
                        std::cout << frame << " " << rect.x / scale << " " << rect.y / scale 
                            << " " << rect.width / scale << " " << rect.height / scale << " " << " " << skin(head) << " " << model << "\n";
                        if(show) {
                            cv::rectangle(copy, rect, cv::Scalar(255, 0, 0), 1);
                            std::stringstream value;
                            value << model;
                            cv::putText(copy, value.str(), cvPoint(rect.x + 5, rect.y + 10), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cvScalar(0, 0, 255));
                        }
                    }
                    model++;
                }
                if(show) {
                    cv::imshow("detector output", copy);
                    cv::waitKey(100);
                }
                return true;
            }

    };

}

int main(int argc, char** argv) {

    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --data <directory>                directory containing haar cascades\n");
    options.AddUsage("  --show                            draw detector output\n");
    options.AddUsage("  --shots <shot-file>               shot segmentation output\n");

    std::string data = options.Get<std::string>("--data", ".");
    bool show = options.IsSet("--show");
    double scale = options.Read("--scale", 1.0);
    std::string shotFilename = options.Get<std::string>("--shots", "");

    amu::VideoReader video;
    if(!video.Configure(options)) {
        return 1;
    }

    if(options.Size() != 0 || shotFilename == "") options.Usage();

    std::vector<amu::ShotSegment> shots;
    amu::ShotSegment::ParseShotSegmentation(shotFilename, shots);

    cv::Mat image;
    amu::Detector detector(data);

    for(size_t shot = 0; shot < shots.size(); shot++) {
        video.Seek(shots[shot].frame);
        video.ReadFrame(image);
        if(false == detector.Detect(image, scale, video.GetIndex(), show)) return 1;
    }

    return 0;
}
