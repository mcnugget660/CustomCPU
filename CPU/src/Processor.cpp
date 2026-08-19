#include "Processor.h"

#include <iostream>

std::array<std::string, 40> Processor::instruction_codes = {
    "NULL", "ADD", "MUL", "MULU", "DIV", "DIVU", "SETL", "SETR", "SET", "SETU",
    "JMPC", "AND", "OR", "XOR", "ADDC", "ADDCH", "MOVS", "MOV", "STAT", "LOAD",
    "STORE", "JMPE", "JMPNE", "JMPG", "JMPGE", "SFTLC", "SFTRC", "SFTRAC", "SFTL",
    "SFTR", "SFTRA", "RET", "JMP", "OUT", "NOP", "KILL", "FORK", "NEG", "NOT", "CEIL"
};

std::array<void (Processor::*)(), 16> Processor::instruction_table1 = {
    &Processor::ROUTE1, &Processor::ADD, &Processor::MUL, &Processor::MULU, &Processor::DIV,
    &Processor::DIVU, &Processor::SETL, &Processor::SETR, &Processor::SET, &Processor::SETU,
    &Processor::JMPC, &Processor::AND, &Processor::OR, &Processor::XOR, &Processor::ADDC, &Processor::ADDCH
};
    
std::array<void (Processor::*)(), 16> Processor::instruction_table2 = {
    &Processor::ROUTE2, &Processor::MOVS, &Processor::MOV, &Processor::STAT, &Processor::LOAD,
    &Processor::STORE, &Processor::JMPE, &Processor::JMPNE, &Processor::JMPG, &Processor::JMPGE,
    &Processor::SFTLC, &Processor::SFTRC, &Processor::SFTRAC,&Processor::SFTL, &Processor::SFTR, &Processor::SFTRA
};
    
std::array<void (Processor::*)(), 9> Processor::instruction_table3 = {
    &Processor::RET, &Processor::JMP, &Processor::OUT, &Processor::NOP, &Processor::KILL,
    &Processor::FORK, &Processor::NEG, &Processor::NOT, &Processor::CEIL
};


#define setStatus(bit) STATUS |= (1 << bit)
#define setStatusTo(bit, value) STATUS = STATUS & (-1 ^ (1 << bit)) | (1 << value);

#define PC sr[0]
#define RETURN sr[1]
#define HI sr[2]
#define LOW sr[3]
#define CLOCK sr[4]
#define STATUS sr[5]
#define IN_R sr[6]
#define OUT_R sr[7]

#define ugr(x) ((uint16_t) gr[x])

// >> is assumed to be arithmetic on signed values and logical on unsigned ones

#define r1 ((instruction<<4)>>12)
#define r2 ((instruction<<8)>>12)
#define r3 ((instruction<<12)>>12)

void Processor::ROUTE1(){(this->*instruction_table2[(instruction<<4)>>12])();}
void Processor::ADD(){uint32_t temp = gr[r2] + gr[r3]; setStatusTo(0, temp & (1 << 16)); gr[r1] = temp;}
void Processor::MUL(){int32_t temp = ((int32_t) gr[r2]) * gr[r3]; HI = temp>>16; LOW = temp; gr[r1] = LOW;}
void Processor::MULU(){uint32_t temp = ((uint32_t) ugr(r2)) * ugr(r3); HI = temp>>16; LOW = temp; gr[r1] = LOW;}
void Processor::DIV(){HI = gr[r2] / gr[r3]; LOW = gr[r2] % gr[r3]; gr[r1] = HI;}
void Processor::DIVU(){HI = ugr(r2) / ugr(r3); LOW = ugr(r2) % ugr(r3); gr[r1] = HI;}
void Processor::SETL(){gr[r1] = ((ugr(r1)<<8)>>8) + (instruction<<8);}
void Processor::SETR(){gr[r1] = ((gr[r1]>>8)<<8) + ((instruction<<8)>>8);}
void Processor::SET(){gr[r1] = (int8_t) ((instruction<<8)>>8);}
void Processor::SETU(){gr[r1] = ((instruction<<8)>>8);}
void Processor::JMPC(){nxt_PC = (int16_t) PC +  (((int16_t)(instruction << 4)) >> 4);}
void Processor::AND(){gr[r1] = gr[r2] & gr[r3];}
void Processor::OR(){gr[r1] = gr[r2] | gr[r3];}
void Processor::XOR(){gr[r1] = gr[r2] ^ gr[r3];}
void Processor::ADDC(){uint32_t temp = gr[r1] + (((int16_t)(instruction << 8)) >> 8);setStatusTo(0,temp & (1 << 16));gr[1] = temp;}
void Processor::ADDCH(){uint32_t temp = gr[r1] + (((int16_t)(instruction << 8)));setStatusTo(0,temp & (1 << 16));gr[1] = temp;}

#undef r1
#undef r2
#define r1 ((instruction<<8)>>12)
#define r2 ((instruction<<12)>>12)

void Processor::ROUTE2(){(this->*instruction_table3[(instruction<<8)>>12])();}
void Processor::MOVS(){gr[r1] = sr[((instruction<<13)>>13)];}
void Processor::MOV(){gr[r1] = gr[r2];}
void Processor::STAT(){gr[r1] = (STATUS) & (1 << r2);}
void Processor::LOAD(){gr[r1] = (*memory)[ugr(r2)];}
void Processor::STORE(){(*memory)[ugr(r2)] = gr[r1];}
void Processor::JMPE(){RETURN = PC + 1; nxt_PC = gr[r2] ? nxt_PC : ugr(r1);}
void Processor::JMPNE(){RETURN = PC + 1; nxt_PC = gr[r2] ? ugr(r1) : nxt_PC;}
void Processor::JMPG(){RETURN = PC + 1; nxt_PC = gr[r2] > 0 ? ugr(r1) : nxt_PC;}
void Processor::JMPGE(){RETURN = PC + 1; nxt_PC = gr[r2] >= 0 ? ugr(r1) : nxt_PC;}
void Processor::SFTLC(){gr[r1] <<= r2;}
void Processor::SFTRC(){gr[r1] = ugr(r1) >> r2;}
void Processor::SFTRAC(){gr[r1] >>= r2;}
void Processor::SFTL(){if(ugr(r2) < 16) gr[r1] <<= gr[r2]; else setStatus(1);}
void Processor::SFTR(){if(ugr(r2) < 16) gr[r1] = ugr(r1) >> r2; else setStatus(1);}
void Processor::SFTRA(){if(ugr(r2) < 16) gr[r1] >>= gr[r2]; else setStatus(1);}

#undef r1
#define r1 ((instruction<<12)>>12)

void Processor::RET(){nxt_PC = RETURN;}
void Processor::JMP(){RETURN = PC + 1; nxt_PC = ugr(r1);}
void Processor::OUT(){OUT_R = gr[r1];setStatus(3);}
void Processor::NOP(){}
void Processor::KILL(){setStatus(5);}
void Processor::FORK(){setStatus(4);}
void Processor::NEG(){gr[r1] = -gr[r1];}
void Processor::NOT(){gr[r1] = ~gr[r1];}
void Processor::CEIL(){gr[r1] = gr[r1] < 0 ? 0 : gr[r1];}


void Processor::fetch() {
    instruction = memory->at(PC);
}

void Processor::execute() {
    nxt_PC = PC + 1;
    (this->*instruction_table1[instruction>>12])();
    PC = nxt_PC;
    CLOCK++;
}


bool Processor::getOut(uint16_t* value) {
    if(STATUS ^ (1 << 3))
        return false;
    setStatusTo(3, 0);
    *value = OUT_R;
    return true;
}

bool Processor::setIn(uint16_t value) {
    if(STATUS & (1 << 2))
        return false;
    STATUS |= (1 << 2);
    IN_R = value;
    return true;
}

bool Processor::shouldFork() {
    if(STATUS ^ (1 << 4))
        return false;
    setStatusTo(4, 0);
    return true;
}

bool Processor::shouldKill() {
    return(STATUS & (1 << 5));
}

void Processor::setUniverse(uint8_t num) {
    STATUS = ((STATUS>>8)<<8) | num;
}

uint8_t Processor::getUniverse() {
    return (STATUS<<8)>>8;
}

Processor::Processor(std::array<int16_t, 641536>* memory) : memory(memory) {
}