#include <iostream>
#include <list>

// http://wiki.tiker.net/OpenCLHowTo
#include <opencv2/gpu/gpu.hpp>
#include "opencv2/ocl/ocl.hpp"

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

    enum DetectorType { DetectorType_CPU, DetectorType_CUDA, DetectorType_OPENCL };

    class Detector {
        private:
            std::vector<cv::CascadeClassifier> detectors;
            std::vector<cv::gpu::CascadeClassifier_GPU> detectors_gpu;
            std::vector<cv::ocl::OclCascadeClassifier> detectors_ocl;
            DetectorType type;
        public:
            Detector(const std::string& path = ".", DetectorType _type = DetectorType_CPU) : type(_type) {
                if(type == DetectorType_CUDA) {
                    cv::gpu::setDevice(0); // init CUDA
                    detectors_gpu.push_back(cv::gpu::CascadeClassifier_GPU(path + "/haarcascade_frontalface_alt.xml"));
                    detectors_gpu.push_back(cv::gpu::CascadeClassifier_GPU(path + "/haarcascade_profileface.xml"));
                } else if(type == DetectorType_OPENCL) {
                    detectors_ocl.push_back(cv::ocl::OclCascadeClassifier());
                    detectors.back().load(path + "/haarcascade_frontalface_alt.xml");
                    //detectors_ocl.push_back(cv::ocl::OclCascadeClassifier());
                    //detectors.back().load(path + "/haarcascade_profileface.xml");
                } else {
                    detectors.push_back(cv::CascadeClassifier(path + "/haarcascade_frontalface_alt.xml"));
                    detectors.push_back(cv::CascadeClassifier(path + "/haarcascade_profileface.xml"));
                    //detectors.push_back(cv::CascadeClassifier(path + "/haarcascade_frontalface_alt2.xml"));
                    //detectors.push_back(cv::CascadeClassifier(path + "/haarcascade_upperbody.xml"));
                    //detectors.push_back(cv::CascadeClassifier(path + "/haarcascade_fullbody.xml"));
                }
            }

            bool Detect(const cv::Mat& image, double scale = 1.0, int frame = 0, bool show = false) {
                cv::Mat copy, gray;
                if(show) image.copyTo(copy);

                cv::cvtColor(image, gray, CV_BGR2GRAY);

                std::vector<cv::Rect> detectedFaces;
                int model = 0;

                if(type == DetectorType_CUDA) {
                    cv::gpu::GpuMat gpuImage;
                    gpuImage.upload(gray);
                    for(std::vector<cv::gpu::CascadeClassifier_GPU>::iterator detector = detectors_gpu.begin(); detector != detectors_gpu.end(); detector++) {
                        if(detector->empty()) {
                            std::cerr << "ERROR: could not load models\n";
                            return false;
                        }

                        detector->visualizeInPlace = false;
                        detector->findLargestObject = false;

                        cv::gpu::GpuMat facesBuffer;
                        cv::Mat facesDownloaded;
                        int numDetections = detector->detectMultiScale(gpuImage, facesBuffer);
                        facesBuffer.colRange(0, numDetections).download(facesDownloaded);

                        detectedFaces.clear();
                        for(size_t i = 0; i < numDetections; i++) {
                            detectedFaces.push_back(facesDownloaded.ptr<cv::Rect>()[i]);
                        }

                        for(std::vector<cv::Rect>::const_iterator i = detectedFaces.begin(); i != detectedFaces.end(); i++) {
                            cv::Mat head(image, *i);
                            std::cout << frame << " " << i->x / scale << " " << i->y / scale 
                                << " " << i->width / scale << " " << i->height / scale << " " << skin(head) << " " << model << "\n";
                            if(show) {
                                cv::rectangle(copy, *i, cv::Scalar(255, 0, 0), 1);
                                std::stringstream value;
                                value << model;
                                cv::putText(copy, value.str(), cvPoint(i->x + 5, i->y + 10), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cvScalar(0, 0, 255));
                            }
                        }
                        model++;
                    }
                } else if(type == DetectorType_OPENCL) {
                    cv::ocl::oclMat oclGray(gray);
                    for(std::vector<cv::ocl::OclCascadeClassifier>::iterator detector = detectors_ocl.begin(); detector != detectors_ocl.end(); detector++) {
                        if(detector->empty()) {
                            std::cerr << "ERROR: could not load models\n";
                            return false;
                        }
                        detectedFaces.clear();
                        detector->detectMultiScale(oclGray, detectedFaces);
                        for(std::vector<cv::Rect>::const_iterator i = detectedFaces.begin(); i != detectedFaces.end(); i++) {
                            cv::Mat head(image, *i);
                            std::cout << frame << " " << i->x / scale << " " << i->y / scale 
                                << " " << i->width / scale << " " << i->height / scale << " " << skin(head) << " " << model << "\n";
                            if(show) {
                                cv::rectangle(copy, *i, cv::Scalar(255, 0, 0), 1);
                                std::stringstream value;
                                value << model;
                                cv::putText(copy, value.str(), cvPoint(i->x + 5, i->y + 10), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cvScalar(0, 0, 255));
                            }
                        }
                        model++;
                    }
                } else {
                    for(std::vector<cv::CascadeClassifier>::iterator detector = detectors.begin(); detector != detectors.end(); detector++) {
                        if(detector->empty()) {
                            std::cerr << "ERROR: could not load models\n";
                            return false;
                        }
                        detectedFaces.clear();
                        detector->detectMultiScale(gray, detectedFaces);
                        for(std::vector<cv::Rect>::const_iterator i = detectedFaces.begin(); i != detectedFaces.end(); i++) {
                            cv::Mat head(image, *i);
                            std::cout << frame << " " << i->x / scale << " " << i->y / scale 
                                << " " << i->width / scale << " " << i->height / scale << " " << skin(head) << " " << model << "\n";
                            if(show) {
                                cv::rectangle(copy, *i, cv::Scalar(255, 0, 0), 1);
                                std::stringstream value;
                                value << model;
                                cv::putText(copy, value.str(), cvPoint(i->x + 5, i->y + 10), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cvScalar(0, 0, 255));
                            }
                        }
                        model++;
                    }
                }
                if(show) {
                    cv::imshow("detector output", copy);
                    cv::waitKey(100);
                }
                std::cout.flush();
                return true;
            }

    };

}

int main(int argc, char** argv) {

    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --data <directory>                directory containing haar cascades\n");
    options.AddUsage("  --show                            draw detector output\n");
    options.AddUsage("  --type <cpu|cuda|opencl>          choose from CPU/GPU-accelerated detector\n");

    std::string data = options.Get<std::string>("--data", ".");
    bool show = options.IsSet("--show");
    double scale = options.Read("--scale", 1.0);
    std::string detectorType = options.Get<std::string>("--type", "cpu");

    amu::VideoReader video;
    if(!video.Configure(options)) {
        return 1;
    }

    if(options.Size() != 0) options.Usage();

    amu::DetectorType type = amu::DetectorType_CPU;
    if(detectorType == "cuda") type = amu::DetectorType_CUDA;
    else if(detectorType == "opencl") type = amu::DetectorType_OPENCL;

    cv::Mat image;
    amu::Detector detector(data, type);

    while (video.HasNext()) {
        //std::cerr << video.GetIndex() << "\n";
        video.ReadFrame(image);
        if(false == detector.Detect(image, scale, video.GetIndex(), show)) return 1;
    }

    return 0;
}
