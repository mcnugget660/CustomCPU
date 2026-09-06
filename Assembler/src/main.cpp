#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <functional>
#include "../include/Assembler.h"
#include <regex>

using namespace std;

int main(int argc, char* argv[]) {
    if(argc == 1) {
        cout << "Error: No File" << endl;
        return 1;
    }

    unsigned int offset = 0;
    bool instSet = false;
    bool little = false;

    for(int i = 2; i < argc; i++) {
        if(argv[i] == "-o") {
            if(i == argc - 1) {
                cout << "Error: no address number" << endl;
                return 1;
            }
            offset = stoi(argv[++i]);
        } else if(argv[i] == "--littleEndian")
            little = instSet = true;
        else if(argv[i] == "--bigEndian") 
            little = !(instSet = true);
    }

    if(instSet)
    return !Assembler(argv[1]).print(regex_replace(argv[1], regex("\\..*"),"") + ".asm", offset, little); // vexing
    return !Assembler(argv[1]).print(regex_replace(argv[1], regex("\\..*"),"") + ".asm", offset); // vexing
}