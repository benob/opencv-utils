#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>

// tesseract
#include <tesseract/baseapi.h>
#include <tesseract/resultiterator.h>

// accurate frame reading
#include "repere.h"
#include "binarize.h"
#include "xml.h"

#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

namespace amu {

    class Rect {
        public:
            double start, end;
            int x, y, width, height;
            std::vector<std::string> text;

            Rect() : start(0), end(0), x(0), y(0), width(0), height(0) { }

            Rect(const Rect& other, double factor = 1) : start(other.start), end(other.end), x(other.x * factor), y(other.y * factor), width(other.width * factor), height(other.height * factor), text(other.text) { }

            Rect(int _x, int _y, int _width, int _height) : start(0), end(0), x(_x), y(_y), width(_width), height(_height) { }

            Rect(double _start, double _end, int _x, int _y, int _width, int _height, std::vector<std::string> _text = std::vector<std::string>()) : start(_start), end(_end), x(_x), y(_y), width(_width), height(_height), text(_text) { }

            operator cv::Rect() {
                return cv::Rect(x, y, width, height);
            }

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

            /*static std::vector<amu::Rect> ReadAll2(std::istream& input) {
                std::vector<amu::Rect> rects;
                Node* root = amu::ParseXML(input);
                if(root == NULL) {
                    return rects;
                }
                root->Print();
                return rects;
            }*/

            static std::vector<amu::Rect> ReadAll(std::istream& input, std::vector<std::string>& tail, int x_offset = 4, int y_offset = -1) {
                std::vector<amu::Rect> rects;
                std::vector<std::string> text;
                double time = -1;
                double duration = -1;
                int x = -1, y = -1, width = -1, height = -1;
                int line_num = 0;
                while(!input.eof()) {
                    std::string line;
                    if(!std::getline(input, line)) break;
                    //std::cerr << line << "\n";
                    line_num ++;
                    if(amu::StartsWith(line, "<Eid ")) {
                        std::string positionString = line.substr(std::string("<Eid name=\"TextDetection\" taskName=\"TextDetectionTask\" position=\"").length());
                        size_t timeEnd = positionString.find('"'); 
                        std::string timeString = positionString.substr(0, timeEnd);
                        time = TimeFromString(timeString);
                        size_t positionStart = timeEnd + std::string("\" position=\"").length();
                        size_t positionEnd = positionString.find('"', positionStart + 1);
                        std::string durationString = positionString.substr(positionStart, positionEnd - positionStart);
                        duration = TimeFromString(durationString);
                    } else if(amu::StartsWith(line, "<IntegerInfo name=\"Position_X\">")) {
                        x = strtol(line.substr(std::string("<IntegerInfo name=\"Position_X\">").length()).c_str(), NULL, 10);
                    } else if(amu::StartsWith(line, "<IntegerInfo name=\"Position_Y\">")) {
                        y = strtol(line.substr(std::string("<IntegerInfo name=\"Position_Y\">").length()).c_str(), NULL, 10);
                    } else if(amu::StartsWith(line, "<IntegerInfo name=\"Width\">")) {
                        width = strtol(line.substr(std::string("<IntegerInfo name=\"Width\">").length()).c_str(), NULL, 10);
                    } else if(amu::StartsWith(line, "<IntegerInfo name=\"Height\">")) {
                        height = strtol(line.substr(std::string("<IntegerInfo name=\"Height\">").length()).c_str(), NULL, 10);
                    }
                    text.push_back(line);
                    if(line == "</Eid>") {
                        if(time == -1 || duration == -1 || x == -1 || y == -1 || width == -1 || height == -1) {
                            std::cerr << "WARNING: incomplete rectangle ending at line " << line_num << "\n";
                        }
                        rects.push_back(amu::Rect(time, time + duration, x + x_offset, y + y_offset, width - x_offset, height - y_offset, text));
                        x = y = width = height = -1;
                        time = -1;
                        duration = -1;
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
        OCR(const std::string& datapath="", const std::string& lang="fra") : imageWidth(0), imageHeight(0) {
            if(datapath != "") {
                setenv("TESSDATA_PREFIX", std::string(datapath + "/../").c_str(), 1);
            }
            tess.Init("OCR", lang.c_str(), tesseract::OEM_DEFAULT); 
            SetMixedCase();
        }

        void SetUpperCase(bool ignoreAccents = false) {
            if(ignoreAccents) 
                tess.SetVariable("tessedit_char_whitelist", "!\"$%&'()+,-./0123456789:;?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\_ «°»€");
            else 
                tess.SetVariable("tessedit_char_whitelist", "!\"$%&'()+,-./0123456789:;?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\_ «°»ÀÂÇÈÉÊËÎÏÔÙÚÛÜ€");
        }

        void SetMixedCase(bool ignoreAccents = false) {
            if(ignoreAccents) 
                tess.SetVariable("tessedit_char_whitelist", "!\"$%&'()+,-./0123456789:;?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\_abcdefghijklmnopqrstuvwxyz «°»€");
            else 
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
            amu::Trim(result.text);
            delete text;
            result.confidence = tess.MeanTextConf() / 100.0;
            return result;
        }

        Result RefineAndProcess(amu::Rect rect, double factor = 1, int y_start = -4, int y_end = 4, int y_step = 2) {
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

    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --data <directory>                tesseract model directory (containing tessdata/)\n");
    options.AddUsage("  --lang <language>                 tesseract model language (default fra)\n");
    options.AddUsage("  --upper-case                      contains only upper case characters\n");
    options.AddUsage("  --ignore-accents                  do not predict letters with diacritics\n");
    options.AddUsage("  --sharpen                         sharpen image before processing\n");
    options.AddUsage("  --show                            show image beeing processed\n");

    double zoom = options.Read("--scale", 1.0); // from video.Configure()
    std::string dataPath = options.Get<std::string>("--data", "");
    std::string lang = options.Get<std::string>("--lang", "fra");
    bool upper_case = options.IsSet("--upper-case");
    bool ignore_accents = options.IsSet("--ignore-accents");
    bool sharpen = options.IsSet("--sharpen");
    bool show = options.IsSet("--show");

    amu::OCR ocr(dataPath, lang);
    if(upper_case) ocr.SetUpperCase(ignore_accents);
    else ocr.SetMixedCase(ignore_accents);

    amu::VideoReader video;
    if(!video.Configure(options)) return 1;
    if(options.Size() != 0) options.Usage();

    std::vector<std::string> remaining;
    std::vector<amu::Rect> rects = amu::Rect::ReadAll(std::cin, remaining);
    //std::vector<amu::Rect> rects = amu::Rect::ReadAll2(std::cin);
    std::stable_sort(rects.begin(), rects.end(), amu::RectLess());
    std::cerr << "read " << rects.size() << " rectangles\n";

    size_t current = 0;
    int frame = 0;

    cv::Mat resized;

    for(current = 0; current < rects.size(); current++) {
        //video.SeekTime((rects[current].start + rects[current].end) / 2);
        //video.SeekTime(rects[current].start);
        //while(video.HasNext() && video.GetTime() <= rects[current].end) {
        amu::Result result;
        result.confidence = 0;
        result.text = "TESSERACT_FAILED";
        double time = (rects[current].start + rects[current].end) / 2;
        video.SeekTime(time);
        if(abs(video.GetTime() - time) < 1 && video.HasNext()) {
            video.ReadFrame(resized);

            cv::Rect rect = amu::Rect(rects[current], zoom);
            if(show) cv::imshow("original", resized(rect));

            if(sharpen) {
                cv::Mat blured;
                cv::GaussianBlur(resized, blured, cv::Size(0, 0), 3);
                cv::addWeighted(resized, 1.5, blured, -0.5, 0, blured);
            }

            cv::Mat cropped = resized(rect);
            
            ocr.SetImage(cropped);
            result = ocr.Process();

            if(show) {
                cv::imshow("binarized", cropped);
                cv::waitKey(0);
            }
            //break;
        }
        std::cerr << frame << " " << video.GetTime() << " " << result.text << "\n";
        for(size_t i = 0; i < rects[current].text.size(); i++) {
            if(amu::StartsWith(rects[current].text[i], "<StringInfo name=\"Text\">")) {
                //std::cout << rects[current].text[i] << "\n";
                //std::cout << "<StringInfo time=\"" << amu::Rect::StringFromTime(time) << "\" name=\"TesseractText\">" << result.text << "</StringInfo>\n";
                std::cout << "<StringInfo name=\"Text\">" << result.text << "</StringInfo>\n";
                std::cerr << frame << " " << video.GetTime() << " " << result.text << "\n";
            } else {
                std::cout << rects[current].text[i] << "\n";
            }
        }
        //std::cerr << frame << "/" << numFrames <<"\n";
    }
    for(size_t i = 0; i < remaining.size(); i++) {
        std::cout << remaining[i] << "\n";
    }
    return 0;
    }
