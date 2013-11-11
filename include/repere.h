#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>

#include "video.h"
#include "idx.h"
#include "shot.h"

namespace amu {

    void Tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters = " ") {
        std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
        std::string::size_type pos     = str.find_first_of(delimiters, lastPos);
        tokens.clear();
        while (std::string::npos != pos || std::string::npos != lastPos)
        {
            tokens.push_back(str.substr(lastPos, pos - lastPos));
            lastPos = str.find_first_not_of(delimiters, pos);
            pos = str.find_first_of(delimiters, lastPos);
        }
    }



}
