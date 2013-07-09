#pragma once

struct Person {
    std::string name;
    int start_frame;
    int end_frame;
    cv::Rect bounding_box;
}
