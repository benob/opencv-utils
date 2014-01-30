#pragma once

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <X11/keysym.h>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include "idx.h"
#include "uem.h"
#include "commandline.h"
#include "utils.h"
#include "librepere.h"

namespace amu {

    enum VideoType { VideoType_None, VideoType_Video, VideoType_ImageList, VideoType_Repere };

    class VideoReader {
        private:
            const Idx* idx;
            Uem uem;
            int index;
            double time;
            bool loaded;
            enum VideoType type;
            int frameSkip;
            bool deinterlace;
            cv::Size size;
            cv::Size lastReadSize;
            std::string showname;
            bool videoFinished;
            int endFrame;
            int endTime;
            RepereExtractKeyframe::repere_video* repereVideo;

            std::map<int, std::string> images;
            std::map<int, std::string>::iterator currentImage;
            std::string currentImageName;
            cv::VideoCapture video;

        public:
            VideoReader(const std::string& filename = "", const std::string& dirname = "") : idx(NULL), index(0), time(0), loaded(false), type(VideoType_None), frameSkip(0), deinterlace(false), size(0, 0), videoFinished(true), endFrame(-1), endTime(-1), repereVideo(NULL) {
                if(filename != "") {
                    if(dirname != "") {
                        LoadImageList(filename, dirname);
                    } else {
                        LoadVideo(filename);
                    }
                }
            }

            void AddUsage(CommandLine& options) {
                options.AddUsage("Options for reading video\n"
                        "  --video <filename>                video file (cannot be used at the same time as image lists)\n"
                        "  --repere-video <filename>         MPG video file read with repere-extract-keyframe, requires idx\n"
                        "  --image-list <filename>           file containing on each line a frame number and an image filename, requires idx\n"
                        "  --image-dir <dirname>             directory name appended to image names (defaults to dir of image-list)\n"
                        "  --idx <filename>                  idx file for mapping frame numbers to times\n"
                        "  --uem <filename>                  file containing a list of segments associated with a show name\n"
                        "  --uem-show <showname>             show name for reading uem (defaults to basename of video/image-list)\n"
                        "  --size <int>x<int>                resize pictures to this size if specified\n"
                        "  --scale <double>                  scale picture according to this factor after resizing (default 1.0)\n"
                        "  --frame-skip <int>                skip n frames after reading a frame (default 0)\n"
                        "  --deinterlace                     discard even lines on picture\n"
                        "  --start <double>                  start video at that time (also applied to uem segments)\n"
                        "  --end <double>                    end video at that time\n"
                        "  --start-frame <int>               start video at that frame number\n"
                        "  --end-frame <int>                 end video at that frame\n");
            }

            bool Configure(CommandLine& options, bool failIfNotLoaded = true) {
                AddUsage(options);
                loaded = false;
                std::string video = options.Get("--video", std::string(""));
                std::string repereVideoFilename = options.Get("--repere-video", std::string(""));
                std::string imageList = options.Get("--image-list", std::string(""));
                std::string imageDir = options.Get("--image-dir", amu::DirName(imageList));
                std::string idx = options.Get("--idx", std::string(""));
                std::string uem = options.Get("--uem", std::string(""));
                showname = options.Get("--uem-show", std::string(""));
                std::string size_string = options.Get("--size", std::string(""));
                if(size_string != "") {
                    size_t xLocation = size_string.find("x");
                    std::stringstream widthReader(size_string);
                    std::stringstream heightReader(size_string.substr(xLocation + 1));
                    int width, height;
                    widthReader >> width;
                    heightReader >> height;
                    if(width <= 0 || height <= 0) {
                        std::cerr << "ERROR: invalid size \"" << size_string << "\"\n";
                        return false;
                    }
                    size = cv::Size(width, height);
                }
                double scale = options.Get<double>("--scale", 1.0);
                if(scale != 1.0) size = cv::Size(size.width * scale, size.height * scale);

                frameSkip = options.Get("--frame-skip", 0);
                deinterlace = options.IsSet("--deinterlace");
                double start = options.Get("--start", 0.0);
                endTime = options.Get("--end", -1);
                int start_frame = options.Get("--start-frame", 0);
                endFrame = options.Get("--end-frame", -1);

                int numActive = 0;
                if(video != "") numActive += 1;
                if(imageList != "") numActive += 1;
                if(repereVideoFilename != "") numActive += 1;
                if(numActive > 1) {
                    std::cerr << "ERROR: can only specify one of video, repere-video and image-list\n";
                    return false;
                }
                if(failIfNotLoaded && numActive == 0) {
                    options.Usage();
                    return false;
                }

                if(imageList != "") LoadImageList(imageList, imageDir);
                else if(video != "") LoadVideo(video);
                else if(repereVideoFilename != "") {
                    if(idx == "") {
                        std::cerr << "ERROR: you must specify an idx with a repere video\n";
                        return false;
                    }
                    LoadRepereVideo(repereVideoFilename, idx);
                }

                if(idx != "") LoadIdx(idx);

                if(showname == "") {
                    if(type == VideoType_Video) showname = amu::ShowName(video);
                    else if(type == VideoType_ImageList) showname = amu::ShowName(imageList);
                    else showname = amu::ShowName(repereVideoFilename);
                }
                if(uem != "") LoadUem(uem, showname);
                if(start != 0.0) SeekTime(start);
                if(start_frame != 0.0) Seek(start_frame);

                return Loaded();
            }

            std::string GetShowName() {
                return showname;
            }

            bool LoadVideo(const std::string& filename) {
                if(video.open(filename) == false) return false;
                type = VideoType_Video;
                loaded = true;
                videoFinished = false;
                return true;
            }

            bool LoadImageList(const std::string& filename, const std::string& dirname) {
                std::ifstream input(filename.c_str());
                if(!input.is_open()) {
                    std::cerr << "ERROR: could not open \"" << filename << "\"\n";
                    return false;
                }
                std::string line;
                while(std::getline(input, line)) {
                    if(line.find(' ') || line.find('\t')) {
                        std::stringstream reader(line);
                        int frame;
                        std::string filename;
                        reader >> frame >> filename;
                        images[frame] = dirname + "/" + filename;
                    } else {
                        images[images.size()] = line; // support for list of images
                    }
                }
                currentImage = images.begin();
                currentImageName = currentImage->second;
                type = VideoType_ImageList;
                loaded = true;
                videoFinished = false;
                return true;
            }

            bool LoadRepereVideo(const std::string& filename, const std::string& idxFilename) {
                av_register_all();
                repereVideo = RepereExtractKeyframe::repere_open(filename.c_str(), idxFilename.c_str());
                if(repereVideo == NULL) return false;
                index = -1; // WARNING: miss first frame
                type = VideoType_Repere;
                loaded = true;
                videoFinished = false;
                return true;
            }

            bool LoadIdx(const std::string& filename) {
                idx = new Idx(filename);
                return idx->Loaded();
            }

            const Idx* GetIdx() {
                return idx;
            }

            bool LoadUem(const std::string& filename, const std::string& showname) {
                uem.Load(filename, showname);
                return uem.Loaded();
            }

            bool Loaded() {
                return loaded;
            }

            cv::Size GetSize() {
                if(size == cv::Size(0, 0)) {
                    if(type == VideoType_Video) {
                        return cv::Size(video.get(CV_CAP_PROP_FRAME_WIDTH), video.get(CV_CAP_PROP_FRAME_HEIGHT));
                    } else if(type == VideoType_ImageList) {
                        if(lastReadSize == cv::Size(0, 0)) {
                            cv::Mat image = cv::imread(currentImage->second);
                            lastReadSize = image.size();
                        }
                        return lastReadSize;
                    } else if(type == VideoType_Repere) {
                        return cv::Size(repereVideo->w_, repereVideo->h_);
                    }
                }
                return size;
            }

            std::string GetFrameName() {
                if(type == VideoType_ImageList) return currentImageName;
                else if(type == VideoType_Video) {
                    std::stringstream name;
                    name << showname << "_" << index;
                    return name.str();
                } else if(type == VideoType_Repere) {
                    std::stringstream name;
                    name << showname << "_" << index;
                    return name.str();
                }
                return "";
            }

            int NumFrames() {
                if(type == VideoType_Video) return video.get(CV_CAP_PROP_FRAME_COUNT);
                else if(type == VideoType_ImageList) return images.size();
                else if(type == VideoType_Repere) return repereVideo->local_index_size;
                return 0;
            }

            bool SeekTime(double time) {
                if(uem.Loaded()) {
                    Uem::Iterator iterator = uem.GetIterator(time);
                    if(uem.IsInvalid(iterator, time)) {
                        if(uem.HasNext(iterator)) {
                            time = uem.GetNextStart(iterator);
                        } else {
                            return false;
                        }
                    }
                }
                if(type == VideoType_ImageList) {
                    if(idx == NULL) {
                        std::cerr << "ERROR: no index loaded\n";
                        return false;
                    }
                    bool result = Seek(idx->GetFrame(time));
                    return result;
                } else if(type == VideoType_Video) {
                    bool result = video.set(CV_CAP_PROP_POS_MSEC, time * 1000);
                    this->index = video.get(CV_CAP_PROP_POS_FRAMES);
                    this->time = video.get(CV_CAP_PROP_POS_MSEC) / 1000.0;
                    return result;
                } else if(type == VideoType_Repere) {
                    return Seek(idx->GetFrame(time));
                }
                return false;
            }

            // WARNING: not constrained by UEM
            bool Seek(int frame) {
                if(type == VideoType_ImageList) {
                    std::map<int, std::string>::iterator found = images.lower_bound(frame);
                    if(found == images.end()) {
                        std::cerr << "WARNING: could not read frame " << frame << "\n";
                        return false;
                    }
                    currentImage = found;
                    index = currentImage->first;
                    if(currentImage != images.end()) currentImageName = currentImage->second;
                    if(idx != NULL) this->time = idx->GetTime(index);
                    else this->time = 0;
                    return true;
                } else if(type == VideoType_Video) {
                    video.set(CV_CAP_PROP_POS_FRAMES, frame);
                    this->index = video.get(CV_CAP_PROP_POS_FRAMES);
                    this->time = video.get(CV_CAP_PROP_POS_MSEC) / 1000.0;
                    return true;
                } else if(type == VideoType_Repere) {
                    if(frame < 0 || frame >= repereVideo->local_index_size) return false;
                    index = frame - 1;
                    return true;
                }
                return false;
            }

            bool HasNext() {
                if(videoFinished) return false;
                if(type == VideoType_ImageList) {
                    std::map<int, std::string>::iterator other = currentImage;
                    other++;
                    return currentImage != images.end() && other != images.end();
                } else if(type == VideoType_Video) {
                    return GetIndex() + 1 < NumFrames();
                } else if(type == VideoType_Repere) {
                    return index >= -1 && index < repereVideo->local_index_size - 1;
                }
                return false;
            }

            int GetIndex() const {
                return index;
            }

            double GetTime() const {
                return time;
            }

            bool ReadFrame(cv::Mat& resized, int interpolation = cv::INTER_LANCZOS4) {
                if(endTime != -1 && time >= endTime) return false;
                if(endFrame != -1 && index >= endFrame) return false;
                if(videoFinished) return false;
                cv::Mat image;
                if(type == VideoType_ImageList) {
                    if(currentImage == images.end()) {
                        videoFinished = true;
                        return false;
                    }
                    image = cv::imread(currentImage->second);
                    index = currentImage->first;
                    if(idx) time = idx->GetTime(index);
                    if(currentImage != images.end()) currentImageName = currentImage->second;
                    if(image.rows == 0) {
                        std::cerr << "ERROR: could not read " << currentImage->second << "\n";
                        return false;
                    }
                    currentImage++;
                } else if(type == VideoType_Video) {
                    index = video.get(CV_CAP_PROP_POS_FRAMES);
                    if(video.read(image) == false || image.empty() || image.rows == 0) {
                        videoFinished = true;
                        return false;
                    }
                    time = video.get(CV_CAP_PROP_POS_MSEC) / 1000.0;
                } else if(type == VideoType_Repere) {
                    index++;
                    time = RepereExtractKeyframe::repere_decode_frame(repereVideo, index + 1);
                    if(time == -1) {
                        videoFinished = true;
                        return false;
                    }
                    time = idx->GetTime(index);
                    image.create(repereVideo->h_, repereVideo->w_, CV_8UC3);
                    for(int y=0; y < repereVideo->h_; y ++) {
                        for (int x=0; x < repereVideo->w_; x++) {
                            image.at<cv::Vec3b>(y, x)[0] = repereVideo->image.data[0][y * repereVideo->w_ * 3 + x * 3 + 2];
                            image.at<cv::Vec3b>(y, x)[1] = repereVideo->image.data[0][y * repereVideo->w_ * 3 + x * 3 + 1];
                            image.at<cv::Vec3b>(y, x)[2] = repereVideo->image.data[0][y * repereVideo->w_ * 3 + x * 3 + 0];
                        }
                    }
                }
                if(deinterlace) {
                    for(int i = 0; i < image.rows - 1; i += 2) {
                        image.row(i).copyTo(image.row(i + 1));
                    }
                }
                if(size == cv::Size(0, 0)) {
                    image.copyTo(resized);
                } else {
                    cv::resize(image, resized, size, interpolation);
                }
                lastReadSize = image.size();
                if(frameSkip != 0) {
                    Seek(index + 1 + frameSkip);
                }
                if(uem.Loaded()) {
                    Uem::Iterator iterator = uem.GetIterator(time);
                    if(uem.IsInvalid(iterator, time)) {
                        if(uem.HasNext(iterator)) {
                            SeekTime(uem.GetNextStart(iterator));
                        } else {
                            return false;
                        }
                    }
                }
                return true;
            }

            ~VideoReader() {
                if(idx != NULL) delete idx;
                if(repereVideo != NULL) RepereExtractKeyframe::repere_close(repereVideo);
            }
    };

    class Player {
        private:
            amu::VideoReader& video;
            bool playing, frameChanged;
            cv::Mat image;
        public:
            Player(amu::VideoReader& _video) : video(_video), playing(true), frameChanged(false) { }

            void Play() {
                playing = true;
            }

            void Pause() {
                playing = false;
            }

            bool Playing() {
                return playing;
            }

            const cv::Mat& CurrentFrame() {
                return image;
            }

            bool Next() {
                if(playing || frameChanged) {
                    if(video.ReadFrame(image) == false) return false;
                    frameChanged = false;
                }
                int key = cv::waitKey(10);
                switch(key) {
                    case XK_space:
                        playing = !playing;
                        break;
                    case XK_Up:
                        frameChanged = true;
                        break;
                    case XK_Down:
                        video.Seek(video.GetIndex() - 2);
                        frameChanged = true;
                        break;
                    case XK_Left:
                        video.Seek(video.GetIndex() - 100);
                        frameChanged = true;
                        break;
                    case XK_Right:
                        video.Seek(video.GetIndex() + 100);
                        frameChanged = true;
                        break;
                    case XK_t:
                        //ShowTiles(video, cv::Size(1024 / 4, 576 / 4));
                        break;
                    case XK_Escape:
                        return false;
                        break;
                }
                return true;
            }

            void Show() {
                cv::imshow("video", image);
            }
    };


}
