#pragma once

#include <sstream>
#include <string>
#include <set>

namespace amu {

    class Segment {
        public:
            double start;
            double end;
            static const Segment nullSegment;
            Segment() : start(-1), end(-1) {}
            Segment(double _start, double _end) : start(_start), end(_end) { }
            Segment(double time) : start(time), end(time) { }
    };

    struct SegmentLess {
        bool operator ()(Segment const& a, Segment const& b) const {
            if(a.start < b.start) return true;
            return false;
        }
    };

    class Uem {
        public:
            typedef std::set<Segment, SegmentLess>::const_iterator Iterator;

            std::set<Segment, SegmentLess> segments;
            bool loaded;

            // WARNING: assumes correct file format
            Uem(const std::string& filename = "", const std::string& showname = "") : loaded(false) {
                if(filename != "" && showname != "") Load(filename, showname);
            }

            bool Load(const std::string& filename, const std::string& showname) {
                loaded = false;
                std::ifstream input(filename.c_str());
                if(!input) {
                    std::cerr << "ERROR: reading " << filename << "\n";
                    return false;
                }
                std::string line;
                while(std::getline(input, line)) {
                    std::stringstream tokenizer(line);
                    std::string show;
                    int channel;
                    double startTime;
                    double endTime;
                    tokenizer >> show >> channel >> startTime >> endTime;
                    if(show == showname) {
                        segments.insert(Segment(startTime, endTime));
                    }
                }
                if(segments.size() == 0) {
                    std::cerr << "ERROR: no segment found for " << showname << " in uem " << filename << "\n";
                    return false;
                }
                loaded = true;
                return true;
            }

            bool Loaded() {
                return loaded;
            }

            Iterator GetIterator(double time) const {
                return segments.lower_bound(Segment(time));
            }

            bool IsInvalid(Iterator found, double time) const {
                while(found != segments.end() && found->end < time) found++;
                return found == segments.end() || !(found->start <= time && found->end >= time);
            }

            bool IsInvalid(double time) const {
                return IsInvalid(GetIterator(time), time);
            }

            bool HasNext(const Iterator found) const {
                Iterator next = found;
                if(next == segments.end()) return false;
                next++;
                return next != segments.end();
            }

            double GetNextStart(Iterator found) const {
                found++;
                return found->start;
            }
    };
}
