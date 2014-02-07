#include <expat.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <tr1/unordered_map>

/* c++ xml reader using expat */
namespace amu {

    struct Node {
        Node* parent;
        std::string name;
        std::string text;
        std::string tail;
        std::tr1::unordered_map<std::string, std::string> attributes;
        std::vector<Node*> children;
        Node(Node* _parent, const std::string&_name) : parent(_parent), name(_name) { }

        void Print(std::string prefix = "") {
            if(parent == NULL) {
                std::cout << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
            }
            if(parent != NULL) {
                std::cout << "<" << name;
                for(std::tr1::unordered_map<std::string, std::string>::const_iterator i = attributes.begin(); i != attributes.end(); i++) {
                    std::cout << " " << i->first << "=\"" << i->second << "\"";
                }
            }
            if(children.size() == 0 && text == "") {
                if(parent != NULL) std::cout << "/>";
            } else {
                if(parent != NULL) {
                    std::cout << ">";
                    std::cout << text;
                }
                for(size_t i = 0; i < children.size(); i++) {
                    children[i]->Print(prefix + "    ");
                }
                if(parent != NULL) std::cout << "</" << name << ">";
            }
            if(parent != NULL) std::cout << tail;
        }

        void Find(const std::string& _name, std::vector<Node*> &output, const std::string& attribute = "", const std::string& value = "") {
            if(name == _name) {
                if(attribute == "" || (value == "" && attributes.find(attribute) != attributes.end()) || attributes[attribute] == value) {
                    output.push_back(this);
                }
            }
            for(size_t i = 0; i < children.size(); i++) {
                children[i]->Find(_name, output, attribute, value);
            }
        }

        Node* Find(const std::string& _name, const std::string& attribute = "", const std::string& value = "") {
            if(name == _name) {
                if(attribute == "" || attributes[attribute] == value) {
                    return this;
                }
            }
            for(size_t i = 0; i < children.size(); i++) {
                Node* found = children[i]->Find(_name, attribute, value);
                if(found) return found;
            }
            return NULL;
        }

        ~Node() {
            for(size_t i = 0; i < children.size(); i++) delete children[i];
        }
    };

    void start_element(void *data, const char *name, const char **attr) {
        Node** current = (Node**) data;
        Node* element = new Node(*current, name);
        element->name = name;
        while(*attr) {
            element->attributes[*attr] = *(attr + 1);
            attr += 2;
        }
        (*current)->children.push_back(element);
        *current = element;
    }

    void end_element(void *data, const char *name) {
        Node** current = (Node**) data;
        *current = (*current)->parent;
    }

    void characters(void* data, const char* chars, int length) {
        Node** current = (Node**) data;
        if((*current)->children.size() > 0) {
            (*current)->children.back()->tail += std::string(chars, length);
        } else {
            (*current)->text += std::string(chars, length);
        }
    }

    Node* ParseXML(std::istream &input) {
        XML_Parser parser = XML_ParserCreate(NULL);
        Node* root = new Node(NULL, "root");
        Node* current = root;
        XML_SetUserData(parser, &current);
        XML_SetElementHandler(parser, start_element, end_element);
        XML_SetCharacterDataHandler(parser, characters);
        std::string line;
        int line_num = 1;
        while(std::getline(input, line)) {
            line += "\n";
            if (XML_Parse(parser, line.c_str(), line.length(), false) == XML_STATUS_ERROR) {
                std::cerr << "ERROR parsing XML at line " << line_num << ": " << XML_ErrorString(XML_GetErrorCode(parser)) << "\n";
                return NULL;
            }
            line_num++;
        };
        if (!XML_Parse(parser, NULL, 0, true)) {
            std::cerr << "ERROR parsing XML\n";
            return NULL;
        }
        XML_ParserFree(parser);
        return root;
    }

    Node* ParseXML(const std::string& filename) {
        std::ifstream input(filename.c_str());
        if(!input) {
            std::cerr << "ERROR parsing " << filename << "\n";
            return NULL;
        }
        return ParseXML(input);
    }


}
