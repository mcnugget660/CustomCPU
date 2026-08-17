#ifndef PROCESSOR_H_
#define PROCESSOR_H_

#include <cstdint>
#include <array>

class Processor {
public:
    Processor(std::array<int16_t, 641536> memory);

    void fetch();

    void execute();
private:
    std::array<int16_t, 641536> memory;

    // Handlers
    std::array<void (Processor::*)(), 16> instruction_table1;
    std::array<void (Processor::*)(), 16> instruction_table2;
    std::array<void (Processor::*)(), 9> instruction_table3;

    // Registers
    int16_t gr[16] = {};
    uint16_t sr[8] = {};
    uint16_t instruction = 0;

    // Invisible values
    uint16_t nxt_PC = 0;

    // Pointers (dont want to rely on loop burried in Processor.cpp)
    uint16_t* ugr[16] = {(uint16_t*)&gr[0],(uint16_t*)&gr[1],(uint16_t*)&gr[2],(uint16_t*)&gr[3],
        (uint16_t*)&gr[4],(uint16_t*)&gr[5],(uint16_t*)&gr[6],(uint16_t*)&gr[7],(uint16_t*)&gr[8],
        (uint16_t*)&gr[9],(uint16_t*)&gr[10],(uint16_t*)&gr[11],(uint16_t*)&gr[12],(uint16_t*)&gr[13],
        (uint16_t*)&gr[14],(uint16_t*)&gr[15]};

    uint16_t* PC = &sr[0];
    uint16_t* RETURN = &sr[1];
    uint16_t* HI = &sr[2];
    uint16_t* LOW = &sr[3];
    uint16_t* CLOCK = &sr[4];
    uint16_t* STATUS = &sr[5];
    uint16_t* IN = &sr[6];
    uint16_t* OUT_R = &sr[7];

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
void ADDI();
void ADDIH();


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
void INV();
void COMP();
void CEIL();
};

#endif // PROCESSOR_H_