#ifndef PROCESSOR_H_
#define PROCESSOR_H_

#include <cstdint>
#include <array>
#include <string>
#include <deque>

class Processor {
public:
    Processor(std::array<char, 641536>* memory);

    void fetch();

    void execute();

    bool getOut(uint16_t& value, bool& type);

    bool setIn(uint16_t value);

    bool shouldKill();

    bool shouldDebug();

    uint16_t memoryAt(uint16_t address) {
        return *((uint16_t*)(memory->data() + ((address>>1)<<1)));
    }

    // Registers
    int16_t gr[16] = {};
    uint16_t sr[8] = {};
    uint16_t instruction = 0;

    static std::array<std::string, 44> instruction_codes;

    std::array<char, 641536>* memory;

private:
    // Invisible values
    uint16_t nxt_PC = 0;

    
    // Handlers
    static std::array<void (Processor::*)(), 16> instruction_table1;
    static std::array<void (Processor::*)(), 16> instruction_table2;
    static std::array<void (Processor::*)(), 13> instruction_table3;

    void ROUTE1();
    void ADD();
    void MUL();
    void MULU();
    void DIV();
    void DIVU();
    void SETL();
    void SETR();
    void SET();
    void SETU();
    void JMPR();
    void AND();
    void OR();
    void XOR();
    void ADDI();
    void JMPB();


    void ROUTE2();
    void MOVS();
    void MOV();
    void STAT();
    void LOAD();
    void STORE();
    void LOADB();
    void LOADBU();
    void CMPG();
    void CMPGE();
    void CMPL();
    void CMPLE();
    void STOREB();
    void SFTL();
    void SFTR();
    void SFTRA();

    void SADDR();
    void JMP();
    void OUT();
    void NOP();
    void KILL();
    void NEG();
    void NOT();
    void CMPE();
    void CMPNE();
    void OUTS();
    void J();
    void OUTC();
    void DEBUG();
};

#endif // PROCESSOR_H_