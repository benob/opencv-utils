#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>

// opencv
#include <cv.h>
#include <highgui.h>

// tesseract
#include <tesseract/baseapi.h>
#include <tesseract/resultiterator.h>

// accurate frame reading
#include "repere.h"

namespace amu {

    class Rect {
        public:
            double start, end;
            int x, y, width, height;
            std::vector<std::string> text;

            Rect() : start(0), end(0), x(0), y(0), width(0), height(0) { }

            Rect(const Rect& other, int factor = 1) : start(other.start), end(other.end), x(other.x * factor), y(other.y * factor), width(other.width * factor), height(other.height * factor), text(other.text) { }

            Rect(int _x, int _y, int _width, int _height) : start(0), end(0), x(_x), y(_y), width(_width), height(_height) { }

            Rect(double _start, double _end, int _x, int _y, int _width, int _height, std::vector<std::string> _text = std::vector<std::string>()) : start(_start), end(_end), x(_x), y(_y), width(_width), height(_height), text(_text) { }

            static double TimeFromString(const std::string &text) {
                // PT1M57.040S
                if(text.length() < 3 || text[0] != 'P' || text[1] != 'T' || text[text.length() - 1] != 'S') {
                    std::cerr << "ERROR: cannot parse time \"" << text << "\"\n";
                    return 0;
                }
                int hLocation = text.find('H');
                int mLocation = text.find('M');
                double hours = 0, minutes = 0, seconds = 0;
                if(hLocation != -1) {
                    hours = strtod(text.c_str() + 2, NULL);
                    if(mLocation != -1) {
                        minutes = strtod(text.c_str() + hLocation + 1, NULL);
                        seconds = strtod(text.c_str() + mLocation + 1, NULL);
                    } else {
                        std::cerr << "ERROR: cannot parse time \"" << text << "\"\n";
                    }
                } else {
                    if(mLocation != -1) {
                        minutes = strtod(text.c_str() + 2, NULL);
                        seconds = strtod(text.c_str() + mLocation + 1, NULL);
                    } else {
                        seconds = strtod(text.c_str() + 2, NULL);
                    }
                }
                return 3600 * hours + 60 * minutes + seconds;
            }

            static std::string StringFromTime(double time) {
                int hours = ((int) time) / 3600;
                int minutes = (((int) time) / 60) % 60;
                double seconds = time - hours * 3600 - minutes * 60;
                std::stringstream output;
                output << "PT";
                if(hours != 0) output << hours << "H";
                if(minutes != 0) output << minutes << "M";
                char* remainder = NULL;
                asprintf(&remainder, "%.03fS", seconds);
                output << remainder;
                free(remainder);
                return output.str();
            }

            static bool StartsWith(const std::string& line, const std::string& pattern) {
                return line.substr(0, pattern.length()) == pattern;
            }

            static std::vector<amu::Rect> ReadAll(std::istream& input, std::vector<std::string>& tail, int x_offset = 4, int y_offset = -1) {
                std::vector<amu::Rect> rects;
                std::vector<std::string> text;
                double time = -1;
                int x = -1, y = -1, width = -1, height = -1;
                int line_num = 0;
                while(!input.eof()) {
                    std::string line;
                    if(!std::getline(input, line)) break;
                    //std::cerr << line << "\n";
                    line_num ++;
                    if(StartsWith(line, "<Eid ")) {
                        std::string timeString = line.substr(std::string("<Eid name=\"TextDetection\" taskName=\"TextDetectionTask\" position=\"").length());
                        timeString = timeString.substr(0, timeString.find('"'));
                        time = TimeFromString(timeString);
                    } else if(StartsWith(line, "<IntegerInfo name=\"Position_X\">")) {
                        x = strtol(line.substr(std::string("<IntegerInfo name=\"Position_X\">").length()).c_str(), NULL, 10);
                    } else if(StartsWith(line, "<IntegerInfo name=\"Position_Y\">")) {
                        y = strtol(line.substr(std::string("<IntegerInfo name=\"Position_Y\">").length()).c_str(), NULL, 10);
                    } else if(StartsWith(line, "<IntegerInfo name=\"Width\">")) {
                        width = strtol(line.substr(std::string("<IntegerInfo name=\"Width\">").length()).c_str(), NULL, 10);
                    } else if(StartsWith(line, "<IntegerInfo name=\"Height\">")) {
                        height = strtol(line.substr(std::string("<IntegerInfo name=\"Height\">").length()).c_str(), NULL, 10);
                    }
                    text.push_back(line);
                    if(line == "</Eid>") {
                        if(time == -1 || x == -1 || y == -1 || width == -1 || height == -1) {
                            std::cerr << "WARNING: incomplete rectangle ending at line " << line_num << "\n";
                        }
                        rects.push_back(amu::Rect(time, time, x + x_offset, y + y_offset, width - x_offset, height - y_offset, text));
                        x = y = width = height = -1;
                        time = -1;
                        text.clear();
                    }
                }
                tail = text;
                return rects;
            }
    };

    struct RectLess {
        bool operator ()(Rect const& a, Rect const& b) const {
            if(a.start < b.start) return true;
            return false;
        }
    };

    class Result {
        public:
            std::string text;
            double confidence;
            Result() : confidence(0) { }
    };

    class OCR {
        int imageWidth, imageHeight;
        tesseract::TessBaseAPI tess; 
        public:
        OCR(const std::string& datapath="") : imageWidth(0), imageHeight(0) {
            if(datapath != "") {
                setenv("TESSDATA_PREFIX", std::string(datapath + "/../").c_str(), 1);
            }
            tess.Init("OCR", "fra", tesseract::OEM_DEFAULT); 
            SetMixedCase();
        }

        void SetUpperCase() {
            tess.SetVariable("tessedit_char_whitelist", "!\"$%&'()+,-./0123456789:;?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\_ «°»ÀÂÇÈÉÊËÎÏÔÙÚÛÜ€");
        }

        void SetMixedCase() {
            tess.SetVariable("tessedit_char_whitelist", "!\"$%&'()+,-./0123456789:;?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\_abcdefghijklmnopqrstuvwxyz «°»ÀÂÇÈÉÊËÎÏÔÙÚÛÜàâçèéêëîïôöùû€");
        }

        ~OCR() {
        }

        void SetImage(cv::Mat& image) {
            tess.SetImage((uchar*) image.data, image.size().width, image.size().height, image.channels(), image.step1());
            imageWidth = image.size().width;
            imageHeight = image.size().height;
        }

        Result Process(cv::Mat& image) {
            SetImage(image);
            return Process();
        }

        void SetRectangle(const Rect& rect) {
            tess.SetRectangle(rect.x, rect.y, rect.width, rect.height);
        }

        static std::string ProtectNewLines(const std::string& text) {
            std::string str = text;
            std::string oldStr = "\n";
            std::string newStr = " ";
            size_t pos = 0;
            while((pos = str.find(oldStr, pos)) != std::string::npos)
            {
                str.replace(pos, oldStr.length(), newStr);
                pos += newStr.length();
            }
            return str;
        }

        Result Process() {
            Result result;
            tess.Recognize(NULL);
            const char* text = tess.GetUTF8Text();
            result.text = ProtectNewLines(std::string(text));
            delete text;
            result.confidence = tess.MeanTextConf() / 100.0;
            return result;
        }

        Result RefineAndProcess(amu::Rect rect, int factor = 1, int y_start = -4, int y_end = 4, int y_step = 2) {
            Result argmax;
            int argmax_q = 0;
            int argmax_d = 0;
            argmax.confidence = -1;
            for(int q = y_start; q <= y_end; q += y_step) {
                for(int d = y_start; d <= y_end; d += y_step) {
                    amu::Rect modified(amu::Rect(rect.x, rect.y + q, rect.width, rect.height + d), factor);
                    if(modified.x < 0) { modified.width += modified.x; modified.x = 0; }
                    if(modified.y < 0) { modified.height += modified.y; modified.y = 0; }
                    if(modified.x + modified.width > imageWidth) modified.width = imageWidth - modified.x;
                    if(modified.y + modified.height > imageHeight) modified.height = imageHeight - modified.x;
                    if(modified.width <= 0 || modified.height <= 0) {
                        std::cerr << "skipping rectangle " << modified.x << " " << modified.y << " " << modified.width << " " << modified.height << "\n";
                        continue;
                    }
                    SetRectangle(modified);
                    Result result = Process();
                    if(result.confidence > argmax.confidence) {
                        //std::cerr << result.confidence << " " << tess.GetUTF8Text() << "\n";
                        argmax = result;
                        argmax_q = q;
                        argmax_d = d;
                    }
                }
            }
            // reposition rectangle in case we need to extract a lattice
            SetRectangle(amu::Rect(amu::Rect(rect.x, rect.y + argmax_q, rect.width, rect.height + argmax_d), factor));
            return argmax;
        }

    };

}

int main(int argc, char** argv) {

    int zoom = 2; // zoom factor
    bool refine = false; // try to find better rectangles 
    bool upper_case = false;
    int y_start = -3, y_end = 3, y_step = 3;
    std::string frameList = "";
    std::string index = "";
    std::string frameDir = "";
    std::string dataPath = "";

    if(argc < 4) {
        std::cerr << "usage: " << argv[0] << " [options] <frame-list> <idx> <frame-dir>" << std::endl;
        std::cerr << "--zoom <int>                  zoom factor before performing OCR (default=" << zoom << ")\n";
        std::cerr << "--refine <from> <to> <step>   refine input rectangle to get better confidence (example -3 3 3)\n";
        std::cerr << "--upper-case                  only allow for upper case characters\n";
        std::cerr << "--data <director>             set model directory\n";
        return 1;
    }

    for(int i = 1; i < argc; i++) {
        std::string argument = argv[i];
        if(argument == "--zoom") {
            zoom = strtol(argv[i + 1], NULL, 10);
            i++;
        } else if(argument == "--refine") {
            y_start = strtol(argv[i + 1], NULL, 10);
            y_end = strtol(argv[i + 2], NULL, 10);
            y_step = strtol(argv[i + 3], NULL, 10);
            refine = true;
            i += 3;
        } else if(argument == "--upper-case") {
            upper_case = true;
        } else if(argument == "--data") {
            dataPath = argv[i + 1];
            i++;
        } else if(frameList == "") {
            frameList = argv[i];
        } else if(index == "") {
            index = argv[i];
        } else if(frameDir == "") {
            frameDir = argv[i];
        }
    }

    std::cerr << "zoom        = " << zoom << "\n";
    std::cerr << "refine      = " << refine << " " << y_start << ":" << y_end << ":" << y_step << "\n";
    std::cerr << "uppercase   = " << upper_case << "\n";
    std::cerr << "frame list  = " << frameList << "\n";
    std::cerr << "index       = " << index << "\n";
    std::cerr << "frame dir   = " << frameDir << "\n";
    std::cerr << "data path   = " << dataPath << "\n";

    amu::OCR ocr(dataPath);
    if(upper_case) ocr.SetUpperCase();

    amu::CommandLine options(argv);
    amu::VideoReader video;
    if(!video.Configure(options)) return 1;

    //int numFrames = capture.get(CV_CAP_PROP_FRAME_COUNT);
    /*int frameWidth = capture.get(CV_CAP_PROP_FRAME_WIDTH);
    int frameHeight = capture.get(CV_CAP_PROP_FRAME_HEIGHT);*/
    amu::Idx idx(index);

    std::vector<std::string> remaining;
    std::vector<amu::Rect> rects = amu::Rect::ReadAll(std::cin, remaining);
    std::stable_sort(rects.begin(), rects.end(), amu::RectLess());
    std::cerr << "read " << rects.size() << " rectangles\n";

    // read rectangles from stdin (see get-rectangles.py companion script) -> proper xml reading would be better
    // process all frames in order (opencv screws up on some videos if we jump to a specific frame
    size_t current = 0;
    int frame = 0;

    cv::Mat resized;

    for(current = 0; current < rects.size(); current++) {
        video.SeekTime(rects[current].start);
        while(video.HasNext() && video.GetTime() <= rects[current].end) {
            video.ReadFrame(resized);
            ocr.SetImage(resized);
            amu::Result result;
            ocr.SetRectangle(amu::Rect(rects[current], zoom));
            if(refine) result = ocr.RefineAndProcess(rects[current], zoom, y_start, y_end, y_step);
            else result = ocr.Process();
            for(size_t i = 0; i < rects[current].text.size(); i++) {
                if(amu::Rect::StartsWith(rects[current].text[i], "<StringInfo name=\"Text\">")) {
                        //std::cout << rects[current].text[i] << "\n";
                        //std::cout << "<StringInfo time=\"" << amu::Rect::StringFromTime(time) << "\" name=\"TesseractText\">" << result.text << "</StringInfo>\n";
                    std::cout << "<StringInfo name=\"Text\">" << result.text << "</StringInfo>\n";
                    std::cerr << frame << " " << video.GetTime() << " " << result.text << "\n";
                } else {
                    std::cout << rects[current].text[i] << "\n";
                }
            }
        }
        //std::cerr << frame << "/" << numFrames <<"\n";
    }
    for(size_t i = 0; i < remaining.size(); i++) {
        std::cout << remaining[i] << "\n";
    }
    return 0;
}
