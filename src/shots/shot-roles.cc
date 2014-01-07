#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "shot-features.h"

namespace amu {

    struct ShotRole {
        std::string show;
        int frame;
        std::string label;

        ShotRole() {}
        ShotRole(const std::string _show, int _frame, std::string _label) : show(_show), frame(_frame), label(_label) {
        }

        static std::map<int, ShotRole> Read(std::istream& input, const std::string targetShow = "") {
            std::map<std::string, std::string> mapping;
            mapping["Journaliste"] = "J";
            mapping["Invité/inteviewé"] = "I";
            mapping["Personnesd\'intêret"] = "A";
            mapping["Autre"] = "A";
            mapping["Presentateur"] = "P";
            mapping["Figurants"] = "A";

            std::map<int, ShotRole> output;
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
                std::string label = "other";
                if(subshots.size() > 1) {
                    label = "mixed";
                } else {
                    const picojson::object& subshot = subshots[0].get<picojson::object>();
                    std::string type = subshot.find("type")->second.get<std::string>();
                    if(amu::StartsWith(type, "Plateau")) label = "set";
                    else if(amu::StartsWith(type, "Reportage")) label = "report";
                }
                label += ":";
                for(picojson::array::const_iterator subshot_iterator = subshots.begin(); subshot_iterator != subshots.end(); subshot_iterator++) {
                    const picojson::object& subshot = subshot_iterator->get<picojson::object>();
                    std::string type = subshot.find("type")->second.get<std::string>();
                    const picojson::array persons = subshot.find("persons")->second.get<picojson::array>();
                    for(picojson::array::const_iterator person_iterator = persons.begin(); person_iterator != persons.end(); person_iterator++) {
                        const picojson::object& person = person_iterator->get<picojson::object>();
                        std::string role = person.find("role")->second.get<std::string>();
                        label += mapping[role];
                    }
                }
                if(targetShow == "" || targetShow == showname) {
                    output[frame] = ShotRole(showname, frame, label);
                }
            }

            return output;
        };

        static std::map<int, ShotRole> Read(const std::string& filename, const std::string targetShow = "") {
            std::ifstream input(filename.c_str());
            if(!input) {
                std::cerr << "ERROR: loading shots from \"" << filename << "\"\n";
                return std::map<int, ShotRole>();
            }
            return Read(input, targetShow);
        }
    };

}

int main(int argc, char** argv) {
    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --shots <shots-file>              shot segmentation (exclusive with --annotations)\n");
    options.AddUsage("  --annotations <annotation-file>   shot reference annotations (exclusive with --shots)\n");
    options.AddUsage("  --labels-only                     only output labels\n");

    std::string shotFile = options.Get<std::string>("--shots", "");
    std::string annotationFile = options.Get<std::string>("--annotations", "");
    bool labelsOnly = options.IsSet("--labels-only");

    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(options.Size() != 0 || (annotationFile == "" && shotFile == "") || (annotationFile != "" && shotFile != "")) options.Usage();

    std::map<int, amu::ShotRole> shotRoles;
    if(annotationFile != "") {
        shotRoles = amu::ShotRole::Read(annotationFile, video.GetShowName());
    } else if(shotFile != "") {
        std::vector<amu::ShotSegment> shots = amu::ShotSegment::Read(shotFile);
        for(size_t i = 0; i < shots.size(); i++) {
            shotRoles[shots[i].frame] = amu::ShotRole(video.GetShowName(), shots[i].frame, "other:");
        }
    }

    if(labelsOnly) {
        for(std::map<int, amu::ShotRole>::const_iterator shot = shotRoles.begin(); shot != shotRoles.end(); shot++) {
            std::cout << shot->second.label << "\n";
        }
    } else {
        amu::FeatureExtractor extractor;

        cv::Mat image;
        for(std::map<int, amu::ShotRole>::const_iterator shot = shotRoles.begin(); shot != shotRoles.end(); shot++) {
            video.Seek(shot->first);
            if(!video.ReadFrame(image) || image.empty()) {
                std::cerr << "ERROR: reading frame " << video.GetIndex() << "\n";
                continue;
            }
            std::vector<float> features = extractor.Compute(image);

            std::cout << shot->second.label;
            for(size_t i = 0; i < features.size(); i++) {
                std::cout << " " << i + 1 << ":" << features[i];
            }
            std::cout << "\n";
        }
    }
    return 0;
}
