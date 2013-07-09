#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iostream>

namespace amu {

    struct FrameLess {
        bool operator ()(std::pair<int, double> const& a, std::pair<int, double> const& b) const {
            if(a.first < b.first) return true;
            return false;
        }
    };

    struct TimeLess {
        bool operator ()(std::pair<double, int> const& a, std::pair<double, int> const& b) const {
            if(a.first < b.first) return true;
            return false;
        }
    };

    class Idx {
        bool loaded;
        public:
        std::vector<std::pair<int, double> > frame2time;
        std::vector<std::pair<double, int> > time2frame;

        // WARNING: assumes correct file format
        Idx(const std::string& filename = "") : loaded(false) {
            if(filename != "") Load(filename);
        }

        bool Load(const std::string& filename) {
            std::ifstream input(filename.c_str());
            if(!input) {
                std::cerr << "ERROR: reading " << filename << "\n";
                return false;
            }
            while(!input.eof()) {
                std::string line;
                if(!std::getline(input, line)) break;
                std::stringstream tokenizer(line);
                int frame = 0;
                std::string type;
                int offset;
                double time = 0;
                tokenizer >> frame >> type >> offset >> time;
                frame2time.push_back(std::pair<int, double>(frame, time));
                time2frame.push_back(std::pair<double, int>(time, frame));
            }
            std::stable_sort(frame2time.begin(), frame2time.end(), amu::FrameLess());
            std::stable_sort(time2frame.begin(), time2frame.end(), amu::TimeLess());
            loaded = true;
            return true;
        }

        bool Loaded() const {
            return loaded;
        }

        int GetFrame(double time) const {
            std::vector<std::pair<double, int> >::const_iterator found = std::lower_bound(time2frame.begin(), time2frame.end(), std::pair<double, int>(time, 0), amu::TimeLess());
            return found->second;
        }

        double GetTime(int frame) const {
            std::vector<std::pair<int, double> >::const_iterator found = std::lower_bound(frame2time.begin(), frame2time.end(), std::pair<double, int>(frame, 0), amu::FrameLess());
            return found->second;
        }
    };

}
