#include "Processor.h"

#include <iostream>

std::array<std::string, 39> Processor::instruction_codes = {
    "NULL", "ADD", "MUL", "MULU", "DIV", "DIVU", "SETL", "SETR", "SET", "SETU",
    "JMPC", "AND", "OR", "XOR", "ADDC", "ADDCH", "MOVS", "MOV", "STAT", "LOAD",
    "STORE", "JMPE", "JMPNE", "JMPG", "JMPGE", "SFTLC", "SFTRC", "SFTRAC", "SFTL",
    "SFTR", "SFTRA", "RET", "JMP", "OUT", "NOP", "KILL", "NEG", "NOT", "CEIL"
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
    
std::array<void (Processor::*)(), 8> Processor::instruction_table3 = {
    &Processor::RET, &Processor::JMP, &Processor::OUT, &Processor::NOP, 
    &Processor::KILL, &Processor::NEG, &Processor::NOT, &Processor::CEIL
};

// Some status bits can only be set to 1
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

#define r1 ((instruction&0b0000111100000000)>>8)
#define r2 ((instruction&0b0000000011110000)>>4)
#define r3 (instruction&0b0000000000001111)

#define co2 (instruction&0b0000000011111111)
#define co3 (instruction&0b0000111111111111)
using namespace std;

// Beware Integer Promotion

void Processor::ROUTE1(){(this->*instruction_table2[(instruction<<4)>>12])();}
void Processor::ADD(){uint32_t temp = gr[r2] + gr[r3]; setStatusTo(0, temp & (1 << 16)); gr[r1] = temp;}
void Processor::MUL(){
    int32_t temp = gr[r2] * gr[r3]; 
    HI = ((uint32_t)temp)>>16; 
    LOW = temp; 
    gr[r1] = LOW;
}
void Processor::MULU(){uint32_t temp = ugr(r2) * ugr(r3); HI = temp>>16; LOW = temp; gr[r1] = LOW;}
void Processor::DIV(){
    if(!gr[r3]) setStatus(1); 
    else {
        HI = gr[r2] / gr[r3]; 
        LOW = gr[r2] % gr[r3]; 
        gr[r1] = HI;
    }
}
void Processor::DIVU(){if(!gr[r3]) setStatus(1); else {HI = ugr(r2) / ugr(r3); LOW = ugr(r2) % ugr(r3); gr[r1] = HI;}}
void Processor::SETL(){gr[r1] = (ugr(r1)&0b0000000011111111) | (instruction<<8);}
void Processor::SETR(){gr[r1] = ((gr[r1]>>8)<<8) | co2;}
void Processor::SET(){gr[r1] = (int8_t) co2;}
void Processor::SETU(){gr[r1] = co2;}
void Processor::JMPC(){nxt_PC = (int32_t) PC + co3;}
void Processor::AND(){gr[r1] = gr[r2] & gr[r3];}
void Processor::OR(){gr[r1] = gr[r2] | gr[r3];}
void Processor::XOR(){gr[r1] = gr[r2] ^ gr[r3];}
void Processor::ADDC(){uint32_t temp = gr[r1] + co2;setStatusTo(0,temp & (1 << 16));gr[1] = temp;}
void Processor::ADDCH(){uint32_t temp = gr[r1] + co2;setStatusTo(0,temp & (1 << 16));gr[1] = temp;}

#undef r1
#undef r2
#define r1 ((instruction&0b0000000011110000)>>4)
#define r2 (instruction&0b0000000000001111)

void Processor::ROUTE2(){(this->*instruction_table3[(instruction<<8)>>12])();}
void Processor::MOVS(){gr[r1] = sr[r2&7];}
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
#define r1 (instruction&0b0000000000001111)

void Processor::RET(){nxt_PC = RETURN;}
void Processor::JMP(){RETURN = PC + 1; nxt_PC = ugr(r1);}
void Processor::OUT(){OUT_R = gr[r1];setStatus(3);}
void Processor::NOP(){}
void Processor::KILL(){setStatus(5);}
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

bool Processor::shouldKill() {
    return(STATUS & (1 << 5));
}

Processor::Processor(std::array<uint16_t, 641536>* memory) : memory(memory) {
}