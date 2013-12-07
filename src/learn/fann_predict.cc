#include <stdio.h>
#include <iostream>
#include <sstream>
#include "floatfann.h"

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cerr << "usage: " << argv[0] << " <model>\n";
        return 1;
    }

    struct fann *ann = fann_create_from_file(argv[1]);

    std::string line;
    while(std::getline(std::cin, line)) {
        int label, index;
        fann_type input[324];
        fann_type* output;

        std::stringstream reader(line);
        reader >> label;
        for(int i = 0; i < 324; i++) {
            reader >> index;
            reader.ignore(1);
            reader >> input[i];
        }
        output = fann_run(ann, input);
        std::cout << label << " " << output[0] << "\n";
    }

    fann_destroy(ann);
    return 0;
}
