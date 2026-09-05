#ifndef ASSEMBLER_H_
#define ASSEMBLER_H_

#include <string>
#include <fstream>
#include <map>
#include <unordered_map>
#include <cstdint>
#include <vector>
#include <unordered_set>
#include <stack>

struct Instruction {
    uint16_t opCode;
    int hb_type[4]; // 0 - nothing, 1 - register, 2 - special register, 3 - constant
    bool isSigned;
};

// Debug information for second-pass errors
struct Position {
    int machinePos;
    std::string filename;
    int defLinePos;
    int placeLinePos;
};

struct Insertion {
    int position;
    int size;
    bool relative; // If position maps to instruction positions or is a literal value
};

struct Scope {
    std::vector<uint16_t> machineCode;
    std::unordered_map<std::string, Scope> namedScopes;
    std::unordered_map<std::string, std::string> macros;
    std::vector<std::pair<Position, Insertion>> codeInsertions;
    std::vector<std::pair<Position, int>> paddingInsertions;

    Scope() {}
};

struct ScopeInfo {
    int line;
    std::string name;
};

struct LabelInfo {
    int pos;
    int lineNumber;
    bool fixed;
};

struct LabelReference {
    Position referencePos;
    int machinePos;
    int size;
    bool fixed;
    bool relative;
};

class Assembler {
public:
    Assembler(std::string filepath);
    bool print(std::string filename, unsigned int instructionOffset, bool littleEndian = isLittleEndian());

    Scope& getMain();
    bool isValid();
private:
    static std::map<std::string, Instruction> insts;
    static std::map<std::string, int> registerConversions;

    // File Scope
    std::ifstream file;
    std::string filename;
    std::vector<int> lineMappings;

    // Scope Construction
    Scope mainScope;
    std::stack<std::pair<Scope*, ScopeInfo>> scopes;
    std::unordered_map<std::string, Scope*> parentScopes;
    std::unordered_map<std::string, std::string> parentMacros;

    // Access Macros in a logical manner
    std::unordered_map<std::string, std::string> temporaryMacros;
    std::unordered_map<std::string, std::vector<std::string>> temporaryMacroGroups;

    // Access scopes in a logical manner
    std::unordered_map<std::string, Scope*> temporaryScopes;
    std::unordered_map<std::string, std::vector<std::string>> temporaryScopeGroups;

    // Relative addressing
    std::vector<std::pair<std::string, LabelReference>> labelReferences;
    std::vector<std::pair<Position, std::pair<int, int>>> lineInsertions;
    std::unordered_map<std::string, LabelInfo> labels;

    // Absolute addressing (to the specific scope)

    // Working Data
    int lineNumber = 0;
    bool valid = false;
    int labelJustPlaced = -1;

    bool processLine(std::string line);
    bool processDirective(std::vector<std::string>& line);
    bool processInstruction(std::vector<std::string>& line, Instruction& rules, uint16_t& result);
    bool processConstant(uint16_t& out, int& size, std::string constant);
    bool finalizeScope();

    bool createLabel(std::string name, uint16_t pointsTo);
    bool checkKeyword(std::string key);
    bool validKeyword(std::string key);
    bool insertCode(Position& pos, Insertion insert, unsigned int instructionOffset);
    bool insertScope(std::string name, std::string prefix);

    bool chainSearch(std::string name, Scope* scope, Scope** out);
    bool getScope(std::string name, Scope** scope);
    bool getMacro(std::string name, std::string* value);

    static bool isLittleEndian() {
        char n = 1;
        return *&n == 1;
    }
};









#endif // ASSEMBLER_H_