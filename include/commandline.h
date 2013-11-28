#pragma once

#include <string>
#include <vector>
#include <cstdlib>
#include <iostream>

namespace amu {

    class CommandLine {
        std::string progname, usage;
        std::vector<std::string> args;

        public:
        CommandLine(char** argv, std::string _usage = "") : usage(_usage) {
            progname = *argv;
            argv++;
            while(*argv != NULL) {
                args.push_back(*argv);
                argv++;
            }
        }

        void Usage(bool leave = true) {
            std::cerr << "usage: " << progname << " " << usage << "\n";
#ifdef VERSION
            std::cerr << "build: " << VERSION << "\n";
#endif
            if(leave) exit(1);
        }

        void AddUsage(const std::string& options) {
            usage += options;
        }

        bool IsSet(const std::string& name) {
            for(std::vector<std::string>::iterator i = args.begin(); i != args.end(); i++) {
                if(*i == name) {
                    args.erase(i);
                    return true;
                }
            }
            return false;
        }

        template <typename T> T Read(const std::string& name, const T& defaultValue) const {
            for(std::vector<std::string>::const_iterator i = args.begin(); i != args.end() && i + 1 != args.end(); i++) {
                if(*i == name) {
                    std::stringstream reader(*(i + 1));
                    T output;
                    reader >> output;
                    return output;
                }
            }
            return defaultValue;
        }

        template <typename T> T Read(int index, const T& defaultValue) {
            if(index >= 0 && index < Size()) {
                std::stringstream reader(args[index]);
                T output;
                reader >> output;
                return output;
            }
            return defaultValue;
        }

        template <typename T> T Get(const std::string& name, const T& defaultValue) {
            for(std::vector<std::string>::iterator i = args.begin(); i != args.end() && i + 1 != args.end(); i++) {
                if(*i == name) {
                    std::stringstream reader(*(i + 1));
                    T output;
                    reader >> output;
                    args.erase(i, i + 2);
                    return output;
                }
            }
            return defaultValue;
        }

        template <typename T> T Get(int index, const T& defaultValue) {
            if(index >= 0 && index < Size()) {
                std::stringstream reader(args[index]);
                T output;
                reader >> output;
                args.erase(args.begin() + index);
                return output;
            }
            return defaultValue;
        }

        size_t Size() const {
            return args.size();
        }

        const std::string& operator[](int index) const {
            if(index < 0) return args[args.size() - index];
            return args[index];
        }

        std::string& operator[](int index) {
            if(index < 0) return args[args.size() - index];
            return args[index];
        }
    };

}
