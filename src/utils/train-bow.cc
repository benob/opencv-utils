#include <iostream>

#include <opencv2/opencv.hpp>

int main(int argc, char** argv) {
    int numDimensions = 1000;
    std::string featureType = "SIFT";

    if(argc <= 1 || argc > 4) {
        std::cerr << "usage: cat <image-list> | " << argv[0] << " <model-name> [num-dimensions=" << numDimensions << "] [features=" << featureType << "]\n";
        return 1;
    }

    std::string modelname = argv[1];
    if(argc >= 3) {
        std::stringstream reader(argv[2]);
        reader >> numDimensions;
    }
    if(argc == 4) featureType = argv[3];

    cv::Ptr<cv::FeatureDetector> featureDetector = cv::FeatureDetector::create(featureType);
    cv::Ptr<cv::DescriptorExtractor> descriptorExtractor = cv::DescriptorExtractor::create(featureType);
    cv::BOWKMeansTrainer bowtrainer(numDimensions);
    std::vector<cv::KeyPoint> keypoints;

    std::cout << "reading images\n";
    std::string filename;
    cv::Mat image, original;
    cv::Mat descriptors;

    while(std::getline(std::cin, filename)) {
        std::cout << filename << "\n";
        cv::Mat original = cv::imread(filename);
        cv::resize(original, image, cv::Size(1024 / 2, 576 / 2));
        featureDetector->detect(image, keypoints);
        descriptorExtractor->compute(image, keypoints, descriptors);
        bowtrainer.add(descriptors);
    }

    std::cout << "clustering\n";
    cv::Mat vocabulary = bowtrainer.cluster();
    std::cout << "saving vocabulary\n";
    cv::FileStorage fs(modelname, cv::FileStorage::WRITE);
    fs << "featureType" << featureType;
    fs << "vocabulary" << vocabulary;
    fs.release();
}
