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
            frame2time.clear();
            time2frame.clear();
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
            }
            std::stable_sort(frame2time.begin(), frame2time.end(), amu::FrameLess());

            // force idx to be monotonic
            size_t i = 0;
            while (i < frame2time.size()) {
                size_t j = i + 1;
                while (j < frame2time.size() && frame2time[j].second < frame2time[i].second) {
                    j += 1;
                }
                if (j > i + 1) {
                    double delta = 0;
                    if (j < frame2time.size()) {
                        delta = (frame2time[j].second - frame2time[i].second) / (frame2time[j].first - frame2time[i].first);
                    }
                    std::pair<int, double> start = frame2time[i];
                    i = i + 1;
                    while (i < j) {
                        frame2time[i] = std::pair<int, double>(frame2time[i].first, start.second + (frame2time[i].first - start.first) * delta);
                        i += 1;
                    }
                }
                i = j;
            }

            for(i = 0; i < frame2time.size(); i++) {
                time2frame.push_back(std::pair<double, int>(frame2time[i].second, frame2time[i].first));
            }
            std::stable_sort(time2frame.begin(), time2frame.end(), amu::TimeLess());
            loaded = true;
            return true;
        }

        bool Loaded() const {
            return loaded;
        }

        // note that in idx, frames start at 1
        int GetFrame(double time) const {
            std::vector<std::pair<double, int> >::const_iterator found = std::lower_bound(time2frame.begin(), time2frame.end(), std::pair<double, int>(time, 0), amu::TimeLess());
            return found->second - 1;
        }

        // note that in idx, frames start at 1
        double GetTime(int frame) const {
            frame += 1;
            std::vector<std::pair<int, double> >::const_iterator found = std::lower_bound(frame2time.begin(), frame2time.end(), std::pair<double, int>(frame, 0), amu::FrameLess());
            return found->second;
        }
    };

}
