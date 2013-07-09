#pragma once

namespace amu {

    struct BOWFeatures {
        std::string featureType;
        cv::Mat vocabulary;
        cv::Ptr<cv::FeatureDetector> featureDetector;
        cv::Ptr<cv::DescriptorExtractor> descriptorExtractor;
        cv::Ptr<cv::DescriptorMatcher> descriptorMatcher;
        cv::BOWImgDescriptorExtractor *bowExtractor;
        bool loaded;

        BOWFeatures(const std::string& modelname) : loaded(false) {
            cv::FileStorage fs(modelname, cv::FileStorage::READ);
            if(!fs.isOpened()) {
                std::cerr << "ERROR: could not load \"" << modelname << "\"\n";
                return;
            }
            cv::Mat vocabulary;
            fs["featureType"] >> featureType;
            fs["vocabulary"] >> vocabulary;
            fs.release();
            featureDetector = cv::FeatureDetector::create(featureType);
            descriptorExtractor = cv::DescriptorExtractor::create(featureType);
            descriptorMatcher = cv::DescriptorMatcher::create("FlannBased");
            bowExtractor = new cv::BOWImgDescriptorExtractor(descriptorExtractor, descriptorMatcher);
            bowExtractor->setVocabulary(vocabulary);
            loaded = true;
        }

        ~BOWFeatures() {
            delete bowExtractor;
        }

        bool IsLoaded() {
            return loaded;
        }

        void Extract(const cv::Mat &image, cv::Mat& descriptors) const {
            std::vector<cv::KeyPoint> keypoints;
            featureDetector->detect(image, keypoints);
            bowExtractor->compute(image, keypoints, descriptors);
        }
    };

}
