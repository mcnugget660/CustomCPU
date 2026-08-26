#ifndef ASSEMBLER_H_
#define ASSEMBLER_H_

#include <string>
#include <fstream>
#include <map>
#include <cstdint>
#include <vector>

struct Instruction {
    uint16_t opCode;
    std::vector<bool> isRegisterGeneral;
};

class Assembler {
public:
    Assembler(std::string filepath, int instructionOffset, bool enableAssemblerCommands, bool littleEndian = isLittleEndian());
    void print(std::string filename);
private:
    static std::map<std::string, Instruction> insts;
    static std::map<std::string, uint16_t> registerConversions;

    std::ifstream file;
    bool enableAssemblerCommands;
    unsigned int instructionOffset;

    std::vector<std::pair<std::string, std::pair<int, std::vector<uint16_t>>>> functionLabels;
    std::map<std::string, std::vector<uint16_t>> processedFunctionLabels;
    std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>> lineReplacement;
    std::map<std::string, std::string> labels;
    std::map<int, int> lineMappings;
    std::vector<uint16_t> machineCode;

    bool valid = false;

    bool processLine(std::string line, int lineNumber);
    bool processDirective(std::vector<std::string>& line, int lineNumber);
    bool processInstruction(uint16_t& result, std::vector<std::string>& line);

    static bool isLittleEndian() {
        char n = 1;
        return *&n == 1;
    }
};









#endif // ASSEMBLER_H_