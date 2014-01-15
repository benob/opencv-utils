#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <map>
#include "repere.h"
#include "picojson.h"

namespace amu {

    enum ShotLabel { ShotLabel_Set, ShotLabel_Report, ShotLabel_Mixed, ShotLabel_Other };

    struct ShotType {
        std::string show;
        int frame;
        ShotLabel label;

        ShotType() {}
        ShotType(const std::string _show, int _frame, ShotLabel _label) : show(_show), frame(_frame), label(_label) {
        }

        static std::map<int, ShotType> Read(std::istream& input, const std::string targetShow = "") {
            std::map<int, ShotType> output;
            picojson::value json;
            input >> json;
            std::string err = picojson::get_last_error();
            if (! err.empty()) {
                std::cerr << err << std::endl;
                return output;
            }

            const picojson::array& annotations = json.get<picojson::array>();
            for (picojson::array::const_iterator annotation = annotations.begin(); annotation != annotations.end(); ++annotation) {
                const picojson::object& o = annotation->get<picojson::object>();
                std::string name = o.find("name")->second.get<std::string>();
                std::stringstream reader(name.substr(name.rfind('_') + 1, name.rfind('.') - name.rfind('_') - 1));
                int frame;
                reader >> frame;
                const picojson::array subshots = o.find("subshots")->second.get<picojson::array>();
                const std::string showname = name.substr(0, name.find('.'));
                ShotLabel label = ShotLabel_Other;
                if(subshots.size() > 1) {
                    label = ShotLabel_Mixed;
                } else {
                    const picojson::object& subshot = subshots[0].get<picojson::object>();
                    std::string type = subshot.find("type")->second.get<std::string>();
                    if(amu::StartsWith(type, "Plateau")) label = ShotLabel_Set;
                    else if(amu::StartsWith(type, "Reportage")) label = ShotLabel_Report;
                }
                if(targetShow == "" || targetShow == showname) {
                    output[frame] = ShotType(showname, frame, label);
                }
            }

            return output;
        };
        static std::map<int, ShotType> Read(const std::string& filename, const std::string targetShow = "") {
            std::ifstream input(filename.c_str());
            if(!input) {
                std::cerr << "ERROR: loading shots from \"" << filename << "\"\n";
                return std::map<int, ShotType>();
            }
            return Read(input, targetShow);
        }
    };

    class FeatureExtractor {
        private:
            cv::HOGDescriptor hog;
            std::vector<std::string> shows;

        public:
            FeatureExtractor() : hog(cv::Size(128, 64), cv::Size(16,16), cv::Size(8,8), cv::Size(8,8), 9) {
                shows.push_back("BFMTV_BFMStory");
                shows.push_back("BFMTV_CultureEtVous");
                shows.push_back("BFMTV_PlaneteShowbiz");
                shows.push_back("LCP_CaVousRegarde");
                shows.push_back("LCP_EntreLesLignes");
                shows.push_back("LCP_LCPInfo13h30");
                shows.push_back("LCP_LCPInfo20h30");
                shows.push_back("LCP_PileEtFace");
                shows.push_back("LCP_TopQuestions");
            }

            std::vector<float> Compute(const cv::Mat& image, const std::string& showname = "") {
                std::vector<float> features;

                cv::Mat hogImage;
                cv::resize(image, hogImage, cv::Size(128, 64), cv::INTER_AREA);

                std::vector<float> hogValues;
                hog.compute(hogImage, hogValues);

                for(size_t i = 0; i < hogValues.size(); i++) {
                    features.push_back(hogValues[i] * 2.0 - 1.0);
                }

                cv::Mat rgbImage;
                cv::resize(image, rgbImage, cv::Size(16, 8), cv::INTER_AREA);

                for(size_t y = 0; y < rgbImage.rows; y++) {
                    for(size_t x = 0; x < rgbImage.cols; x++) {
                        features.push_back((rgbImage.at<cv::Vec3b>(y, x)[0] - 128.0) / 128.0);
                        features.push_back((rgbImage.at<cv::Vec3b>(y, x)[1] - 128.0) / 128.0);
                        features.push_back((rgbImage.at<cv::Vec3b>(y, x)[2] - 128.0) / 128.0);
                    }
                }

                /*for(size_t i = 0; i < shows.size(); i++) {
                    if(showname == shows[i]) features.push_back(1);
                    else features.push_back(-1);
                }*/

                return features;
            }
    };
}

