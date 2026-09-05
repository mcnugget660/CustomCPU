#include "../include/Assembler.h"

#include <iostream>
#include <vector>
#include <assert.h>
#include <regex>
#include <filesystem>
#include <exception>

using namespace std;

map<string, Instruction> Assembler::insts = {
    {"ADD", {1<<8, {0, 1, 1, 1}, true}}, 
    {"MUL", {2<<8, {0, 1, 1, 1}, true}}, 
    {"MULU", {3<<8, {0, 1, 1, 1}, false}},
    {"DIV", {4<<8, {0, 1, 1, 1}, true}}, 
    {"DIVU", {5<<8, {0, 1, 1, 1}, false}}, 
    {"SETL", {6<<8, {0, 1, 3, 3}, false}},
    {"SETR", {7<<8, {0, 1, 3, 3}, false}}, 
    {"SET", {8<<8, {0, 1, 3, 3}, true}}, 
    {"SETU", {9<<8, {0, 1, 3, 3}, false}},
    {"JMPR", {10<<8, {0, 3, 3, 3}, true}},
    {"AND", {11<<8, {0, 1, 1, 1}, false}}, 
    {"OR", {12<<8, {0, 1, 1, 1}, false}},
    {"XOR", {13<<8, {0, 1, 1, 1}, false}}, 
    {"ADDI", {14<<8, {0, 1, 3, 3}, true}}, 
    {"JMPB", {15<<8, {0, 3, 3, 3}, true}},
    {"MOVS", {1<<4, {0, 0, 1, 2}, false}}, 
    {"MOV", {2<<4, {0, 0, 1, 1}, false}}, 
    {"STAT", {3<<4, {0, 0, 1, 3}, false}},
    {"LOAD", {4<<4, {0, 0, 1, 1}, false}}, 
    {"STORE", {5<<4, {0, 0, 1, 1}, false}}, 
    {"LOADB", {6<<4, {0, 0, 1, 1}, false}},
    {"LOADBU", {7<<4, {0, 0, 1, 1}, false}}, 
    {"CMPG", {8<<4, {0, 0, 1, 1}, false}}, 
    {"CMPGE", {9<<4, {0, 0, 1, 1}, false}},
    {"CMPL", {10<<4, {0, 0, 1, 1}, false}},
    {"CMPLE", {11<<4, {0, 0, 1, 1}, false}}, 
    {"STOREB", {12<<4, {0, 0, 1, 1}, false}},
    {"SFTL", {13<<4, {0, 0, 1, 1}, false}}, 
    {"SFTR", {14<<4, {0, 0, 1, 1}, false}}, 
    {"SFTRA", {15<<4, {0, 0, 1, 1}, true}},
    {"SADDR", {0, {0, 0, 0, 1}, false}},
    {"JMP", {1, {0, 0, 0, 1}, false}}, 
    {"OUT", {2, {0, 0, 0, 1}, false}}, 
    {"NOP", {3, {0, 0, 0, 0}, false}},
    {"KILL", {4, {0, 0, 0, 0}, false}}, 
    {"NEG", {5, {0, 0, 0, 1}, false}},
    {"NOT", {6, {0, 0, 0, 1}, false}}, 
    {"CMPE", {7, {0, 0, 0, 1}, false}},
    {"CMPNE", {8, {0, 0, 0, 1}, false}},
    {"OUTS", {9, {0, 0, 0, 2}, false}},
    {"J", {10, {0, 0, 0, 1}, false}},
    {"OUTC", {11, {0, 0, 0, 1}, false}},
    {"DEBUG", {12, {0, 0, 0, 0}, false}}
};

map<string, int> buildRegConversions() {
    map<string, int> temp;
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
    temp["PC"] = 0;
    temp["ADDRESS"] = 1;
    temp["HI"] = 2;
    temp["LOW"] = 3;
    temp["CLOCK"] = 4;
    temp["STATUS"] = 5;
    temp["IN"] = 6;
    temp["OUT"] = 7;
    return temp;
}

map<string, int> Assembler::registerConversions = buildRegConversions();

#define error(ln, errMsg) ((cout << filename << ":" << ln << "> \033[31mError\033[0m: " << errMsg << endl), false);
#define errorC(path, errMsg) ((cout << (path) << "> \033[31mError\033[0m: " << (errMsg) << endl), false);
#define warning(ln, wrrMsg) (cout << filename << ":" << ln << "> \033[34mWarning\033[0m: " << wrrMsg << endl);
#define currentScope (*scopes.top().first)
#define currentScopeInfo (scopes.top().second)

void reverseBits(uint16_t& in) {
    in = (in>>(unsigned int)8) || (in<<8);
    in = ((in & 0b0000111100001111) << 4) || ((in >> (unsigned int)4) & 0b0000111100001111);
    in = ((in & 0b0011001100110011) << 2) || ((in >> (unsigned int)2) & 0b0011001100110011);
    in = ((in & 0b0101010101010101) << 1) || ((in >> (unsigned int)1) & 0b0101010101010101);
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
    return !regex_search(str.cbegin() + pos, str.cend(), m, regex(val)) ? str.size() : m.position() + pos;
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

Assembler::Assembler(string filename) : file(filename), filename(filename), lineMappings(1, -1) {
    if(!file.is_open()) {
        cout << "Error: Invalid file " << filename << endl;
        return;
    }

    scopes.emplace(&mainScope, ScopeInfo{0, "main"});

    string line;
    lineNumber = 0;
    while(getline(file, line)) {
        lineNumber++;
        try {
        if(!processLine(line)) {
            cout << "--------------------> " << trim(line) << endl;
            return;
        }
        } catch(...) {
            error(lineNumber, "c++ exception");
            return;
        }
        // cout << lineNumber << ": " << lineMappings[lineNumber] << endl;
    }

    if(scopes.size() > 1) {
        for(auto &p = scopes.top(); scopes.size(); scopes.pop()) 
        error(p.second.line, "scope \"" << p.second.name << "\" not closed");
        return;
    }

    valid = finalizeScope();
}

Scope& Assembler::getMain() {
    if(!valid)
        throw std::runtime_error("main is invalid");
    return mainScope;
}

bool Assembler::insertCode(Position& pos, Insertion insert, unsigned int instructionOffset) {
    int value = insert.position;
    // Assume all reference locations are intentional - this means that arbitrary insertions would break this

    if(!insert.relative) {
        if(!insert.relative && value == -1)
            value = currentScope.machineCode.size();
        else if(!insert.relative && value == -2)
            value = instructionOffset;
    }
    value *= 2; // Memory is byte aligned

    if(value >= (1<<(4*insert.size)) || value < -(1<<(4*insert.size)))
        return errorC(filename + ":" + to_string(pos.placeLinePos) + "::" + pos.filename + ":" + to_string(pos.defLinePos), "reference resolution can not fit in this instruction");
    
    // All constant parameters are right aligned
    currentScope.machineCode[pos.machinePos] |= value & ((1 << (4 * insert.size)) - 1);
    return true;
}

bool Assembler::print(string outputFile, unsigned int instructionOffset, bool littleEndian) {
    if(!valid)
        return false;

    for(auto cp : mainScope.codeInsertions)
        insertCode(cp.first, cp.second, instructionOffset);

    for(auto cp : mainScope.paddingInsertions) {
        for(int i = (cp.second - cp.first.machinePos % cp.second) % cp.second; i > 0; i--)
            mainScope.machineCode.insert(mainScope.machineCode.begin() + cp.first.machinePos, insts["NOP"].opCode);
    }

    valid = false; // Padding is a final step

    ofstream out(outputFile, ios_base::binary);
    out.write((char*) mainScope.machineCode.data(), sizeof(uint16_t) * mainScope.machineCode.size());
    out.close();
    return true;
}

bool Assembler::isValid() {
    return valid;
}

bool Assembler::chainSearch(string name, Scope* parent, Scope** out) {
    string first = name.substr(0, bfind(name, "\\."));
    if(!validKeyword(first)) return false;
    if(!parent->namedScopes.count(first))
        return error(lineNumber, "scope " + first + " from chain does not exist");
    if(name == first)
        (*out) = &(parent->namedScopes[first]);
    return chainSearch(name.substr(first.size(), name.size() - first.size()), &(parent->namedScopes[first]), out);
}

bool Assembler::getScope(string name, Scope** scope) {
    string first = name.substr(0, bfind(name, "\\."));
    if(temporaryScopes.count(first)) {
        if(first == name)
        return *scope = temporaryScopes[name], true;
        return chainSearch(name.substr(first.size(), name.size() - first.size()), temporaryScopes[first], scope);
    }
    if(parentScopes.count(name)) {
        if(first == name)
        return *scope = parentScopes[name], true;
        return chainSearch(name.substr(first.size(), name.size() - first.size()), parentScopes[first], scope);
    }
    return error(lineNumber, "scope \"" + name + "\" doesn't exist");
}

bool Assembler::getMacro(string name, string* value) {
    if(temporaryMacros.count(name)) 
        return *value = temporaryMacros[name], true;
    if(parentMacros.count(name)) 
        return *value = parentMacros[name], true;
    int pos = name.find_last_of('.');
    if(pos != string::npos) {
        Scope* sp;
        if(!getScope(name.substr(0, pos), &sp)) return false;
        if(!sp->macros.count(name.substr(pos + 1, name.size() - pos - 1)))
            return false;
        *value = sp->macros[name.substr(pos + 1, name.size() - pos - 1)];
        return true;
    }
    return true; // Function is called on every single word
}

// no spaces allowed in constant
bool Assembler::processConstant(uint16_t& out, int& size, string constant) {
    assert(constant.find(" ") == string::npos && constant.size());
    int convert;

    if((constant[0] == 'b' || constant[0] == 'B') && constant.size() > 1) {
        convert = stoi(constant.substr(1), nullptr, 2);
    } else if((constant[0] == 'h' || constant[0] == 'H') && constant.size() > 1) {
        convert = stoi(constant.substr(1), nullptr, 16);
    } else if(constant[0] == '\'') {
        if(constant.size() != 3) return error(lineNumber, "Invalid character literal");
        convert = constant[1];
    } else if(bfind(constant, "[^0-9-]") != constant.size()) {
        return error(lineNumber, "Error: Invalid constant format: \'" << constant << "\'");
    } else
        convert = stoi(constant);

    if(convert >= (1<<12) || convert < -(1<<11)) {
        size = 4;
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

bool Assembler::finalizeScope() {
    for(auto& [name, ref] : labelReferences) {
        if(!labels.count(name))
            return error(ref.referencePos.defLinePos, "Label \"" + name + "\" does not exist in scope");
        if(ref.fixed && !labels[name].fixed)
            return error(ref.referencePos.defLinePos, "Label \"" + name + "\" is not fixed");
        ref.machinePos += labels[name].pos;
        ref.referencePos.placeLinePos = labels[name].lineNumber;
        if(!ref.relative && (ref.machinePos < 0 || ref.machinePos >= currentScope.machineCode.size()))
            return error(ref.referencePos.defLinePos, "reference points outside of scope");
        currentScope.codeInsertions.emplace_back(ref.referencePos, Insertion{ref.machinePos, ref.size, ref.relative});
    }
    labelReferences.clear();
    labels.clear();

    // Insertion
    for(auto& [pos, info] : lineInsertions) {
        // Negative numbers map to specific values
        int location = info.first < 0 ? info.first : lineMappings[info.first];
        if(info.first >= lineNumber)
            return errorC(filename + ":" + to_string(pos.placeLinePos) + "::" + pos.filename + ":" + to_string(pos.defLinePos), "line " + to_string(info.first) + " exceeds scope");
        currentScope.codeInsertions.emplace_back(pos, Insertion{location, info.second, false});
    }
    lineInsertions.clear();

    for(auto& [mc, value] : currentScope.macros)
        parentMacros.erase(mc);

    for(auto& [mc, value] : currentScope.namedScopes)
        parentScopes.erase(mc);

    for(string &s : temporaryMacroGroups[currentScopeInfo.name])
        parentMacros.erase(s);
    for(string &s : temporaryScopeGroups[currentScopeInfo.name])
        parentScopes.erase(s);
    temporaryMacroGroups.erase(currentScopeInfo.name);
    temporaryScopeGroups.erase(currentScopeInfo.name);
    
    if(labelJustPlaced != -1)
        warning(lineNumber - 1, "No instruction follows label");

    return true;
}

bool Assembler::checkKeyword(string key) {
    if(registerConversions.count(key))
        return error(lineNumber, "name " + key + " is a register!");

    if(labels.count(key))
        return error(lineNumber, "name " + key + " is reserved on line " + to_string(labels[key].lineNumber));
    if(parentMacros.count(key))
        return error(lineNumber, "name " + key + " is reserved by a macro");
    if(parentScopes.count(key))
        return error(lineNumber, "name " + key + " is reserved by a scope");

    return validKeyword(key);
}

bool Assembler::validKeyword(string key) {
    if(!key.size())
        return error(lineNumber, "keyword is empty");
    if(tolower(key[0]) == 'h' || tolower(key[0]) == 'b' || key[0] == '#')
        return error(lineNumber, "name can not start with \'" << key[0] << "\'");

    smatch match;
    if(regex_search(key, match, regex{"[^a-zA-Z0-9_]"}), !match.empty())
        return error(lineNumber, "name \"" << key << "\" can not contain \'" << key[match.position()] << "\'");
    return true;
}

bool Assembler::createLabel(string name, uint16_t pointsTo) {
    if(!checkKeyword(name)) return false;
    if(labelJustPlaced != -1) warning(labelJustPlaced, "redundant label \"" + name + "\", immediately follows another label");
    labels[name] = {pointsTo, lineNumber, (currentScope.paddingInsertions.size(), currentScope.paddingInsertions.size() && currentScope.paddingInsertions.back().first.machinePos == pointsTo)};
    labelJustPlaced = lineNumber;
    return true;
}

bool Assembler::processDirective(vector<string>& groups) {
    bool invalidArguments = false;
    if(groups[0] == "#Align16" && !(invalidArguments = groups.size() == 1)) {
        currentScope.paddingInsertions.emplace_back(Position{(int)currentScope.machineCode.size(), filename, lineNumber}, 16);
    } else if(groups[0] == "#Align256" && !(invalidArguments = groups.size() == 1)) {
        currentScope.paddingInsertions.emplace_back(Position{(int)currentScope.machineCode.size(), filename, lineNumber}, 256);
    } else if(groups[0] == "#undef" && !(invalidArguments = groups.size() == 1)) {
        if(!currentScope.macros.count(groups[1]))
            return error(lineNumber, "current scope does not contain macro \"" << groups[1] << "\"");
        currentScope.macros.erase(groups[1]);    
        parentMacros.erase(groups[1]);         
    } else if(groups[0] == "#data" && !(invalidArguments = groups.size() != 2)) {
        currentScope.machineCode.emplace_back();
        int sz;
        if(!processConstant(currentScope.machineCode.back(), sz, groups[1])) return false;
        if(sz > 4) return error(lineNumber, "data exceeds 16 bits");
    } else if(groups[0] == "#dataf" && !(invalidArguments = groups.size() != 4)) {
        if(!createLabel(groups[3], currentScope.machineCode.size())) return false;
        ifstream dataFile(groups[1]);
        if(!dataFile.is_open()) 
            return error(lineNumber, "datafile \"" + groups[1] + "\" doesn't exists!");
        size_t numBytes = filesystem::file_size(groups[1]);
        for(int i = 0; i < numBytes / 2; i++) {
            currentScope.machineCode.emplace_back();
            dataFile >> currentScope.machineCode.back();
        }
    } else if(groups[0] == "#define" && !(invalidArguments = (groups.size() == 1 | groups.size() > 3))) {
        if(!checkKeyword(groups[1])) return false;
        if(groups.size() == 3)
            parentMacros[groups[1]] = currentScope.macros[groups[1]] = groups[2];
        else {
            scopes.emplace(&currentScope.namedScopes[groups[1]], ScopeInfo{lineNumber, groups[1]});
            // Scope is not accessable until after definition
        }
    } else if(groups[0] == "#endif" && !(invalidArguments = groups.size() > 1)) {
        if(scopes.size() <= 1)
            return error(lineNumber, "unassociated endif");

        if(!finalizeScope()) return false;

        parentScopes[currentScopeInfo.name] = scopes.top().first;
        
        scopes.pop();
    } else if(groups[0] == "#using" && !(invalidArguments = groups.size() != 2)) {     
        Scope* scope;
        if(!getScope(groups[1], &scope)) return false;      
        for(auto& [name, ptr] : scope->namedScopes) {
            temporaryScopes[name] = &ptr;
            temporaryScopeGroups[currentScopeInfo.name].emplace_back(name);
        }
        for(auto& [name, value] : scope->macros) {
            temporaryMacros[name] = value;
            temporaryMacroGroups[currentScopeInfo.name].emplace_back(name);
        }
    } else {
        if(!(invalidArguments |= groups.size() != 2 && groups.size() != 4)) {
            if(groups[0] == "#include" || groups[0] == "#insertf" || groups[0] == "#insert") {
                if(groups[0] == "#include" || groups[0] == "#insertf") {
                    Assembler otherFile(groups[1]);
                    Scope& imported = otherFile.getMain();
                    if(!otherFile.isValid()) return false;

                    if(groups.size() == 4) { 
                        if(!checkKeyword(groups[3])) return false;
                        if(currentScope.namedScopes.count(groups[3]))
                            warning(lineNumber, "scope \"" + groups[3] +"\" already exits and will be overwriten");
                        currentScope.namedScopes[groups[3]] = imported;
                    } else {
                        if(imported.machineCode.size())
                            warning(lineNumber, "instructions from file \"" + groups[1] + "\" are not copied");
                        currentScope.namedScopes.insert(imported.namedScopes.begin(), imported.namedScopes.end());
                        currentScope.macros.insert(imported.macros.begin(), imported.macros.end());
                    }
                }
                if(groups[0] == "#insertf" || groups[0] == "#insert") {
                    if(!(invalidArguments |= groups[0] == "#insert" && groups.size() != 2))
                        if(!insertScope(groups[1], "")) return false;
                }
            }
        }
        if(!invalidArguments) {
            return error(lineNumber, "invalid directive");
        } else
            return error(lineNumber, groups[0] << " does not accept " << (groups.size() == 2 ? "" : to_string(groups.size())) << "argument" << "s");
    }
    return true;
}

bool Assembler::insertScope(string name, string prefix) {
    Scope* imported;
    if(!getScope(name, &imported)) return false;
    lineMappings[lineNumber] = currentScope.machineCode.size();
    for(auto cp : imported->codeInsertions)
        currentScope.codeInsertions.emplace_back(Position{cp.first.machinePos + (int) currentScope.machineCode.size(), cp.first.filename, cp.first.defLinePos, cp.first.placeLinePos}, Insertion{cp.second.position + (int) currentScope.machineCode.size(), cp.second.size, cp.second.relative});
    for(auto cp : imported->paddingInsertions)
        currentScope.paddingInsertions.emplace_back(Position{cp.first.machinePos + (int) currentScope.machineCode.size(), cp.first.filename, cp.first.defLinePos, cp.first.placeLinePos}, cp.second);
    currentScope.machineCode.insert(currentScope.machineCode.end(), imported->machineCode.begin(), imported->machineCode.end());

    return true;
}

bool Assembler::processLine(string line) {
    if(line.find("##") != string::npos)
        line = line.substr(0, line.find("##"));
    line = trim(line);

    lineMappings.push_back(-1);

    if(!line.size()) return true;
    vector<string> groups = splitString(line);

    // Insert Macros
    for(int i = 0; i < groups.size(); i++) {
        if(!getMacro(groups[i], &groups[i])) return false;
    }

    if(groups.size() == 1 && groups[0].find("()") == groups[0].size() - 2)
        return insertScope(groups[0].substr(0, groups[0].size() - 2), "");

    if(groups.size() == 1 && groups[0].find(":") == groups[0].size() - 1)
        return createLabel(groups[0].substr(0, groups[0].size() - 1), currentScope.machineCode.size());

    if(line[0] == '#') {
        if(labelJustPlaced != -1 && line != "#endif") 
            warning(labelJustPlaced, "label precededs a definition");
        labelJustPlaced = -1;
        return processDirective(groups);
    }

    if(!insts.count(groups[0]))
        return error(lineNumber, "Invalid insturction \"" << groups[0] << "\"");

    lineMappings[lineNumber] = currentScope.machineCode.size();
    currentScope.machineCode.emplace_back();
    
    labelJustPlaced = -1;
    return processInstruction(groups, insts[groups[0]], currentScope.machineCode.back());
}

bool Assembler::processInstruction(vector<string>& line, Instruction& rules, uint16_t& result) {
    result = rules.opCode << 4;

    int size = 2 - !rules.hb_type[1] - !rules.hb_type[2];
    int inConstSize = (rules.hb_type[1] == 3) + (rules.hb_type[2] == 3) + (rules.hb_type[3] == 3);

    int index = 1;
    for(int i = 0; i < 4; i++) {
        if(!rules.hb_type[i]) continue;
        if(index == line.size()) return error(lineNumber, "instruction " + line[0] + "has too few arguments");
        if(rules.hb_type[i] == 3) break;
        if(!registerConversions.count(line[index]))
            return error(lineNumber, "unknown register \'" << line[index] << "\'");
        
        result |= registerConversions[line[index]] << (4 * (3 - i));
        index++;
    }

    if(index < line.size() && inConstSize) {
        if(line[index] == "#ENDP") {
            lineInsertions.push_back({Position{(int)currentScope.machineCode.size() - 1, filename, lineNumber}, {-1, inConstSize}});
        } else if(line[index] == "#STARTP") {
            lineInsertions.push_back({Position{(int)currentScope.machineCode.size() - 1, filename, lineNumber}, {-2, inConstSize}});
        } else if(line[index][0] == '#') {
            if(line[index].size() == 1)
                return error(lineNumber, "line number must follow #");

            int ln = stoi(line[index].substr(1));
            if(line[index][1] == '+' || line[index][1] == '-')
                ln += lineNumber;
            if(ln <= scopes.top().second.line) 
                return error(lineNumber, "line " << ln << " precedes current scope");
            lineInsertions.emplace_back(Position{(int)currentScope.machineCode.size() - 1, filename, lineNumber}, pair<int, int>{ln, inConstSize});
        } else if(line[index][0] == '$') {
            int offset = line[0].find("JMPR") == string::npos ? 0 : - (currentScope.machineCode.size() - 1);
            if(line[index].find("[") != string::npos && line[index].find("]") != string::npos) {
                line[index] = line[index].substr(0, line[index].find("["));
                if(line[index].find("]") - line[index].find("[") == 1) return error(lineNumber, "[] must include a value");
                offset = stoi(line[index].substr(line[index].find("["), line[index].find("]") - line[index].find("[") - 1));
            }
            line[index] = line[index].substr(1);
            if(!line[index].size()) return error(lineNumber, "Label field is empty!");
            labelReferences.emplace_back(line[index], LabelReference{Position{(int)currentScope.machineCode.size() - 1, filename, lineNumber}, offset, inConstSize, line[0] == "JMPB", line[0] == "JMPR"});
        } else {
            uint16_t constant;
            int constant_size;
            if(!processConstant(constant, constant_size, line[index])) return false;
            if(constant_size > inConstSize) {
                int maxv = (1<<(inConstSize*4 - 1)) - 1, minv = ~maxv;
                if(!rules.isSigned) {
                    minv = 0;
                    maxv = maxv * 2 + 1;
                }
                return error(lineNumber, "constant value is too large for command " << line[0] << " (range of [" << minv << ", " << maxv << "])");
            }
            result |= constant & (inConstSize == 1 ? 0b0000000000001111 : (inConstSize == 2 ? (0b0000000011111111) : (0b0000111111111111)));
        }
        index++;
    }

    if(index < line.size())
        return error(lineNumber, "instruction " + line[0] + " has too many arguments");

    return true;
}