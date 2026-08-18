#ifndef PROCESSOR_H_
#define PROCESSOR_H_

#include <cstdint>
#include <array>

class Processor {
public:
    Processor(std::array<int16_t, 641536>* memory);

    void fetch();

    void execute();

    bool setOut(uint16_t* value);

    void setIn(uint16_t value);

    bool shouldFork();

    bool shouldKill();

    void setUniverse(uint8_t num);

    uint8_t getUniverse();
private:
    std::array<int16_t, 641536>* memory;

    // Registers
    int16_t gr[16] = {};
    uint16_t sr[8] = {};
    uint16_t instruction = 0;

    // Invisible values
    uint16_t nxt_PC = 0;
    
    // Handlers
    static std::array<void (Processor::*)(), 16> instruction_table1;
    static std::array<void (Processor::*)(), 16> instruction_table2;
    static std::array<void (Processor::*)(), 9> instruction_table3;

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
void JMPC();
void AND();
void OR();
void XOR();
void ADDC();
void ADDCH();


void ROUTE2();
void MOVS();
void MOV();
void STAT();
void LOAD();
void STORE();
void JMPE();
void JMPNE();
void JMPG();
void JMPGE();
void SFTLC();
void SFTRC();
void SFTRAC();
void SFTL();
void SFTR();
void SFTRA();

void RET();
void JMP();
void OUT();
void NOP();
void KILL();
void FORK();
void NEG();
void NOT();
void CEIL();
};

#endif // PROCESSOR_H_