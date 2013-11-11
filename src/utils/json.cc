#include <iostream>

namespace amu {

class Json {
    size_t location;
    const char* text;

    char Read() {
        char read = text[location];
        if(read != '\0') {
            location++;
        }
        return read;
    }

    public:
    Json(const char* _text) : text(_text) { }

    void Error(const char* expected) {
        std::cerr << "Error: expected " << expected << "\n";
    }

    int Object() {
        if(Read() == '{') {
            if(Members()) {
            } else if(Read() == '}') {
            } else {
            }
        }
        Error("start of object ({)");
    }

    int Array() {
    }

    int Value() {
    }

    std::string String() {
        if(input
    }

    double Number() {
        double read;
        input >> read;   
        return read;
    }

};

}

int main(int argc, char** argv) {

}
