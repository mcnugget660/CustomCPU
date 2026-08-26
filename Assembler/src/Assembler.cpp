#include "../include/Assembler.h"

#include <iostream>
#include <vector>
#include <assert.h>
#include <regex>

using namespace std;

#define vb vector<bool>
map<string, Instruction> Assembler::insts = {
    // Machine Instructions
    {"ADD", {1<<8, vb({true, true, true})}}, 
    {"MUL", {2<<8, vb({true, true, true})}}, 
    {"MULU", {3<<8, vb({true, true, true})}},
    {"DIV", {4<<8, vb({true, true, true})}}, 
    {"DIVU", {5<<8, vb({true, true, true})}}, 
    {"SETL", {6<<8, vb({true})}},
    {"SETR", {7<<8, vb({true})}}, 
    {"SET", {8<<8, vb({true})}}, 
    {"SETU", {9<<8, vb({true})}},
    {"JMPC", {10<<8, vb({})}},
    {"AND", {11<<8, vb({true, true, true})}}, 
    {"OR", {12<<8, vb({true, true, true})}},
    {"XOR", {13<<8, vb({true, true, true})}}, 
    {"ADDI", {14<<8, vb({true})}}, 
    {"ADDIH", {15<<8, vb({true})}},
    {"MOVS", {1<<4, vb({true, false})}}, 
    {"MOV", {2<<4, vb({true, true,})}}, 
    {"STAT", {3<<4, vb({true})}},
    {"LOAD", {4<<4, vb({true, true})}}, 
    {"STORE", {5<<4, vb({true, true})}}, 
    {"JMPE", {6<<4, vb({true, true})}},
    {"JMPNE", {7<<4, vb({true, true})}}, 
    {"JMPG", {8<<4, vb({true, true})}}, 
    {"JMPGE", {9<<4, vb({true, true})}},
    {"SFTLC", {10<<4, vb({true})}},
    {"SFTRC", {11<<4, vb({true})}}, 
    {"SFTRAC", {12<<4, vb({true})}},
    {"SFTLU", {13<<4, vb({true, true})}}, 
    {"SFTRU", {14<<4, vb({true, true})}}, 
    {"SFTRA", {15<<4, vb({true, true})}},
    {"RET", {0, vb({})}},
    {"JMP", {1, vb({true})}}, 
    {"OUT", {2, vb({true})}}, 
    {"NOP", {3, vb({})}},
    {"KILL", {4, vb({})}}, 
    {"NEG", {6, vb({true})}},
    {"NOT", {7, vb({true})}}, 
    {"CEIL", {8, vb({true})}}
};

map<string, uint16_t> buildRegConversions() {
    map<string, uint16_t> temp;
    for(int i = 0; i < 16; i++) {
        temp["r" + to_string(i)] = i;
        temp["R" + to_string(i)] = i;
        temp["gr" + to_string(i)] = i;
        temp["GR" + to_string(i)] = i;
    }
    for(int i = 0; i < 8; i++) {
        temp["s" + to_string(i)] = i;
        temp["S" + to_string(i)] = i;
        temp["sr" + to_string(i)] = i;
        temp["SR" + to_string(i)] = i;
    }
    return temp;
}

map<string, uint16_t> Assembler::registerConversions = buildRegConversions();

void reverseBits(uint16_t& in) {
    in = (in>>(unsigned int)8) || (in<<8);
    in = ((in & 0b0000111100001111) << 4) || ((in >> (unsigned int)4) & 0b0000111100001111);
    in = ((in & 0b0011001100110011) << 2) || ((in >> (unsigned int)2) & 0b0011001100110011);
    in = ((in & 0b0101010101010101) << 1) || ((in >> (unsigned int)1) & 0b0101010101010101);
}

Assembler::Assembler(string filename, int instructionOffset, bool enableAssemblerCommands, bool littleEndian) : 
file(filename), instructionOffset(instructionOffset), enableAssemblerCommands(enableAssemblerCommands) {
    if(!file.is_open()) {
        cout << "Error: Invalid file " << filename << endl;
        return;
    }

    string line;
    int index = 0;
    while(getline(file, line)) {
        if(!processLine(line, index++)) {
            cout << "at " << filename << ":" << index << " -> " << line << endl;
            return;
        }
    }

    if(functionLabels.size()) {
        for(auto &[p, v] : functionLabels)
        cout << "Error on line " << v.first << " : \"" << p << "\" multiline label not closed" << endl;
        return;
    }
    // Final Pass
    for(auto& [ml, rl] : lineReplacement) {
        int location = lineMappings[rl.first];
        if(location >= (1<<rl.second)) {
            cout << "Error on line " << ml.first << " : the resolution of #" << rl.first << " can not fit in this instruction" << endl;
            return;
        }
        // All constant parameters are right aligned
        machineCode[ml.second] |= location;
    }
    // Flip Endianess
    if(littleEndian != isLittleEndian())
        for(int i = 0; i < machineCode.size(); i++)
            reverseBits(machineCode[i]);
    valid = true;
}

string trim(string str) {
    smatch m1, m2;
    regex_search(str, m1, regex("\\S"));
    regex_search(str, m2, regex("\\S$"));
    return m1.empty() ? "" : str.substr(m1.position(), m2.position() - m1.position() + 1);
}

int bfind(string str, string val, int pos = 0) {
    //assert(pos < str.size());
    smatch m;
    regex_search(str.cbegin() + pos, str.cend(), m, regex(val));
    return m.empty() ? str.size() : m.position() + pos;
}

// Groups can not be empty
vector<string> splitString(string str) {
    vector<string> groups;
    int st = 0;
    while(st != str.size()) {
        int pos = bfind(str, "\\s", st);
        groups.push_back(str.substr(st, pos - st));
        st = bfind(str, "\\S", pos);
    }
    return groups;
}

// no spaces allowed in constant
bool processConstant(uint16_t& out, int& size, string constant) {
    assert(constant.find(" ") == string::npos && constant.size());
    int convert;

    if((constant[0] == 'b' || constant[0] == 'B') && constant.size() > 1) {
        convert = stoi(constant.substr(1), nullptr, 2);
    } else if((constant[0] == 'h' || constant[0] == 'H') && constant.size() > 1) {
        convert = stoi(constant.substr(1), nullptr, 16);
    } else if(bfind(constant, "\\D") != constant.size()) {
        cout << "Error: Invalid constant format: \'" << constant << "\'" << endl;
        return false;
    } else
    convert = stoi(constant);
    if(convert >= (1<<12) || convert < -(1<<11)) {
        size = 4; // No parsing error but will be caught later on
    } else if(convert >= (1<<8) || convert < -(1<<7))
        size = 3;
    else if(convert >= (1<<4) || convert < -(1<<3))
        size = 2;
    else
        size = 1;

    out = (int16_t) convert;
    /*
    int clone = convert;
    convert = abs(convert);
    do {
        size++;
        convert >> 4;
    } while(convert > 0);
    // Negative numbers take one extra bit unless a power of two
    size += ((1<<(4*size-1)) > -clone && (clone < 0));
    */
    return true;
}

bool isComment(string x) {return (x.size() > 1 && x[0] == '#' && x[1] != '#');}

bool Assembler::processDirective(vector<string>& groups, int lineNumber) {
    bool invalidArguments = false;
    if(groups[0] == "#undef") {
        if(!(invalidArguments = groups.size() != 2 && !(groups.size() > 2 && isComment(groups[2]))))
        labels.erase(groups[1]);
    } else if(groups[0] == "#data") {
            
    } else if(groups[0] == "#define" && !(invalidArguments = (groups.size() == 1 || groups.size() > 3 && !isComment(groups[3])))) {
        if(insts.count(groups[1]) < 0) {
            cout << "Error: reserved name for label \"" << groups[1] << "\"" << endl;
            return false;
        } else if(tolower(groups[1][0]) == 'h' || tolower(groups[1][0]) == 'b') {
            cout << "Error: label can not start with \'" << groups[1][0] << "\'" << endl;
            return false;
        }
        if(groups.size() == 3 && !isComment(groups[2]))
            labels[groups[1]] = groups[2];
        else
            functionLabels.push_back({groups[1], {lineNumber, vector<uint16_t>()}});
    } else {
        cout << "Error: invalid directive" << endl;
        return false;
    }
    if(groups[0] == "#endif") {
        if(groups.size() > 1 && !isComment(groups[1])) {
            cout << "Error: endif does not accept a second argument" << endl;
            return false;
        } else if(!functionLabels.size()) {
            cout << "Error: unassociated endif" << endl;
            return false;
        }
        processedFunctionLabels[functionLabels[functionLabels.size() - 1].first] = functionLabels[functionLabels.size() - 1].second.second;
        functionLabels.pop_back();
    }
    if(invalidArguments) {
        cout << "Error: directive \"" << groups[0] << "\" does not accept " << groups.size() << "argument" << (groups.size() == 2 ? "" : "s") << endl;
        return false;
    } else if(groups[0].size() == 1 || groups[0][1] != '#') {
        cout << "Error: invalid directive \"" << groups[0] << "\"" << endl;
        return false;
    }
    return true;
}

bool Assembler::processLine(string line, int lineNumber) {
    line = trim(line);
    if(!line.size()) return true;
    vector<string> groups = splitString(line);

    if(line[0] == '#') {
        lineMappings[lineNumber] = -1;
        return processDirective(groups, lineNumber);
    } else if(insts.count(groups[0])) {
        vector<uint16_t> instructionList(1);
        if(!processInstruction(instructionList[0], groups)) return false;

        for(uint16_t instruction : instructionList) {
            if(functionLabels.size())
                functionLabels[functionLabels.size() - 1].second.second.push_back(instruction);
            else {
                lineMappings[lineNumber] = machineCode.size();
                machineCode.push_back(instruction);
            }
        }
    } else {
        int index = 0;
        do {
            if(!processedFunctionLabels.count(groups[index])) {
                cout << "Error: unkown symbol \"" << groups[index] << "\"" << endl;
                return false;
            }
            lineMappings[lineNumber] = machineCode.size();
            machineCode.insert(machineCode.end(), processedFunctionLabels[groups[index]].begin(), processedFunctionLabels[groups[index]].end());
        } while(groups.size() > index && !isComment(groups[index + 1]));
    }
    return true;
}

bool Assembler::processInstruction(uint16_t& result, vector<string>& line) {
    auto rules = insts[line[0]];
    result = (rules.opCode << 4);

    int size = 2 - (rules.opCode < 16) - (rules.opCode < 256);
    int inConstSize = size + 1 - rules.isRegisterGeneral.size();
    int expectedArgs = rules.isRegisterGeneral.size() + (bool)inConstSize;

    if(line.size() - 1 != expectedArgs) {
        cout << "Error: wrong number of arguments for instruction " << line[0] << " (Expected " << expectedArgs << ")" << endl;
        return false;
    }

    int index = 1;
    for(;index - 1 < rules.isRegisterGeneral.size() && size >= 0; index++, size--) {
        if(rules.isRegisterGeneral[index - 1] && !registerConversions.count(line[index])) {
            cout << "Error: unknown register \'" << line[index] << "\'" << endl;
            return false;
        }
        result += registerConversions[line[index]] << (4 * size);
    }
    if(index < line.size()) {
        uint16_t constant;
        int constant_size;
        if(!processConstant(constant, constant_size, line[index])) return false;
        if(constant_size > inConstSize) {
            int maxv = (1<<(inConstSize*4 - 1)) - 1, minv = ~maxv;
            if(line[0][line[0].size() - 1] == 'U') { // All commands(in above list) follow this convention
                minv = 0;
                maxv = maxv * 2 + 1;
            }
            cout << "Error: constant value is too large for command " << line[0] << " (range of [" << minv << ", " << maxv << "])" << endl;
            return false;
        }
        result += constant;
    }
    return true;
}


void Assembler::print(string outputFile) {
    if(!valid) return;
    ofstream out(outputFile, ios_base::binary);
    out.write((char*) machineCode.data(), sizeof(uint16_t) * machineCode.size());
    out.close();
}