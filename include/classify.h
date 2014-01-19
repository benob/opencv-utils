#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>

namespace amu {

    class LabelMapping {
            bool loaded;
            std::map<std::string, int> mapping;
            std::vector<std::string> labels;
            std::string notFoundNull;

        public:

            LabelMapping(const std::string filename = "") : loaded(false), notFoundNull("") {
                if(filename != "") Load(filename);
            }

            bool Loaded() const {
                return loaded;
            }

            int Map(const std::string label) {
                std::map<std::string, int>::const_iterator found = mapping.find(label);
                if(found == mapping.end()) {
                    mapping[label] = labels.size();
                    labels.push_back(label);
                    return labels.size() - 1;
                } else {
                    return found->second;
                }
            }

            const std::string& Map(int label) const {
                if(label < 0 || label >= labels.size()) return NotFound();
                return labels[label];
            }

            const std::string& NotFound() const {
                return notFoundNull;
            }

            bool Load(std::ifstream& input) {
                mapping.clear();
                std::string line;
                while(std::getline(input, line)) {
                    std::stringstream reader(line);
                    std::string label;
                    int id;
                    reader >> id >> label;
                    mapping[label] = id;
                }
                labels.clear();
                labels.resize(mapping.size());
                for(std::map<std::string, int>::const_iterator i = mapping.begin(); i != mapping.end(); i++) {
                    labels[i->second] = i->first;
                }
                loaded = true;
                return true;
            }

            bool Load(const std::string& filename) {
                std::ifstream input(filename.c_str());
                if(!input) {
                    std::cerr << "ERROR: loading label mapping from \"" << filename << "\"\n";
                    return false;
                }
                return Load(input);
            }

            bool Save(std::ofstream& output) const {
                for(std::map<std::string, int>::const_iterator i = mapping.begin(); i != mapping.end(); i++) {
                    output << i->first << " " << i->second;
                }
                return true;
            }

            bool Save(const std::string& filename) const {
                std::ofstream output(filename.c_str());
                if(!output) {
                    std::cerr << "ERROR: saving label mapping to \"" << filename << "\"\n";
                    return false;
                }
                return Save(output);
            }
    };

    class CumulativeDecision {
        private:
            int numDecisions;
            std::map<std::string, double> scores;

        public:
            CumulativeDecision() : numDecisions(0) { }

            void Add(const std::string& label, double score = 1) {
                scores[label] += score;
                numDecisions++;
            }

            void Add(const std::pair<std::string, double>& decision) {
                Add(decision.first, decision.second);
            }

            std::pair<std::string, double> Best() const {
                double max = 0;
                std::string argmax = "";

                for(std::map<std::string, double>::const_iterator i = scores.begin(); i != scores.end(); i++) {
                    //std::cout << i->first << " " << i->second << "\n";
                    if(argmax == "" || i->second > max) {
                        argmax = i->first;
                        max = i->second;
                    }
                }

                return std::pair<std::string, double>(argmax, max / numDecisions);
            }
    };

    class LibLinearClassifier {
        private:
            bool loaded;
            std::string solverType;
            std::vector<int> labels;
            int numLabels;
            int numFeatures;
            double bias;
            std::vector<std::vector<double> > weights;
            LabelMapping labelMapping;

        public:
            LibLinearClassifier(const std::string& filename = "", const std::string mappingFilename = "") : loaded(false) { 
                if(filename != "") Load(filename, mappingFilename);
            }

            bool Loaded() const {
                return loaded;
            }

            /* model format: labels on 1st line, then weight per label of each feature on following lines */
            bool Load(std::istream& input) {
                labels.clear();
                weights.clear();
                loaded = false;
                std::string line;
                bool readingWeights = false;
                while(std::getline(input, line)) {
                    std::vector<std::string> tokens = amu::Tokenize<std::string>(line);
                    if(readingWeights) {
                        for(size_t i = 0; i < numLabels; i++) weights[i].push_back(amu::Parse<double>(tokens[i]));
                    } else {
                        if(tokens[0] == "solver_type") {
                            solverType = tokens[1];
                        } else if(tokens[0] == "nr_class") {
                            numLabels = amu::Parse<int>(tokens[1]);
                        } else if(tokens[0] == "label") {
                            for(size_t i = 0; i < numLabels; i++) {
                                labels.push_back(amu::Parse<int>(tokens[i + 1]));
                            }
                        } else if(tokens[0] == "nr_feature") {
                            numFeatures = amu::Parse<int>(tokens[1]);
                        } else if(tokens[0] == "bias") {
                            bias = amu::Parse<double>(tokens[1]);
                        } else if(tokens[0] == "w") {
                            readingWeights = true;
                            for(size_t i = 0; i < numLabels; i++) {
                                weights.push_back(std::vector<double>());
                            }
                        }
                    }
                }
                loaded = true;
                return true;
            }

            bool Load(const std::string& filename, const std::string& mappingFilename = "") {
                if(mappingFilename != "") {
                    labelMapping.Load(mappingFilename);
                }
                std::ifstream input(filename.c_str());
                if(!input) {
                    std::cerr << "ERROR: loading model from \"" << filename << "\"\n";
                    return false;
                }
                return Load(input);
            }

            void ComputeScores(const std::vector<float>& descriptors, std::vector<double>& scores) const {
                scores.clear();
                scores.resize(numLabels, 0.0);
                if(!loaded) return;
                for(size_t label = 0; label < numLabels; label++) {
                    for(size_t descriptor = 0; descriptor < numFeatures; descriptor++) {
                        scores[label] += weights[label][descriptor] * descriptors[descriptor];
                    }
                }
                if(bias != 0) {
                    for(size_t label = 0; label < numLabels; label++) {
                        scores[label] += weights[label][numFeatures] * bias;
                    }
                }
                for(size_t label = 0; label < numLabels; label++) {
                    scores[label] = 1.0 / (1.0 + exp(-scores[label]));
                }
            }

            std::pair<std::string, double> ClassifyAndScore(const std::vector<float>& descriptors) const {
                if(!loaded) return std::pair<std::string, double>("", 0);
                double max = 0, min = 0, sum = 0;
                int argmax = -1;
                std::vector<double> scores;
                ComputeScores(descriptors, scores);
                for(size_t label = 0; label < numLabels; label++) {
                    if(argmax == -1 || scores[label] > max) {
                        max = scores[label];
                        argmax = label;
                    }
                    if(scores[label] < min) min = scores[label];
                }
                for(size_t label = 0; label < numLabels; label++) {
                    sum += scores[label] - min;
                    //std::cout << "    " << scores[label] - min << "\n";
                }
                //std::cout << max << " " << min << " " << sum << "\n";
                double normalized = max; //(max - min) / sum;
                if(labelMapping.Loaded()) return std::pair<std::string, double>(labelMapping.Map(labels[argmax]), normalized);
                else return std::pair<std::string, double>(ToString(labels[argmax]), normalized);
            }

            std::string Classify(const std::vector<float>& descriptors) const {
                return ClassifyAndScore(descriptors).first;
            }
    };
}

