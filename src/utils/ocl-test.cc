
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/ocl/ocl.hpp"

int main(int argc, char** argv) {
    cv::Mat image = cv::imread(argv[1]);
    cv::ocl::oclMat gpuImage(image);
    if(gpuImage.empty()) {
        return 1;
    }
    return 0;
}
