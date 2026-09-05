#include "Processor.h"

#include <iostream>

std::array<std::string, 44> Processor::instruction_codes = {
    "NULL", "ADD", "MUL", "MULU", "DIV", "DIVU", "SETL", "SETR", "SET", "SETU",
    "JMPR", "AND", "OR", "XOR", "ADDI", "JMPB", "MOVS", "MOV", "STAT", "LOAD",
    "STORE", "RSTORE", "RLOAD", "CMPG", "CMPGE", "CMPL", "CMPLE", "STOREB", "SFTL",
    "SFTR", "SFTRA", "SADDR", "JMP", "OUT", "NOP", "KILL", "NEG", "NOT", "CMPE", "CMPNE", "OUTS", "J",
    "OUTC", "DEBUG"
};

std::array<void (Processor::*)(), 16> Processor::instruction_table1 = {
    &Processor::ROUTE1, &Processor::ADD, &Processor::MUL, &Processor::MULU, &Processor::DIV,
    &Processor::DIVU, &Processor::SETL, &Processor::SETR, &Processor::SET, &Processor::SETU,
    &Processor::JMPR, &Processor::AND, &Processor::OR, &Processor::XOR, &Processor::ADDI, &Processor::JMPB
};
    
std::array<void (Processor::*)(), 16> Processor::instruction_table2 = {
    &Processor::ROUTE2, &Processor::MOVS, &Processor::MOV, &Processor::STAT, &Processor::LOAD,
    &Processor::STORE, &Processor::LOADB, &Processor::LOADBU, &Processor::CMPG, &Processor::CMPGE,
    &Processor::CMPL, &Processor::CMPLE, &Processor::STOREB,&Processor::SFTL, &Processor::SFTR, &Processor::SFTRA
};
    
std::array<void (Processor::*)(), 13> Processor::instruction_table3 = {
    &Processor::SADDR, &Processor::JMP, &Processor::OUT, &Processor::NOP, &Processor::KILL, 
    &Processor::NEG, &Processor::NOT, &Processor::CMPE, &Processor::CMPNE, &Processor::OUTS, &Processor::J,
    &Processor::OUTC, &Processor::DEBUG
};

// Some status bits can only be set to 1
#define setStatus(bit) (STATUS |= (1 << bit))
#define setStatusTo(bit, value) (STATUS = (STATUS & (-1 ^ (1 << bit))) | ((value != 0) << bit))

#define PC sr[0]
#define ADDRESS sr[1]
#define HI sr[2]
#define LOW sr[3]
#define CLOCK sr[4]
#define STATUS sr[5]
#define IN_R sr[6]
#define OUT_R sr[7]

#define CARRY_BIT 0
#define ARITHMETIC_EXCEPTION_BIT 1
#define IN_WAIT_BIT 2
#define OUT_WAIT_BIT 3
#define OUT_TYPE_BIT 4
#define KILL_BIT 5
#define COMPARE_BIT 6
#define DEBUG_BIT 7

#define COMPARE_VALUE (STATUS & (1 << COMPARE_BIT))

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
void Processor::ADD(){uint32_t temp = gr[r2] + gr[r3]; setStatusTo(CARRY_BIT, temp & (1 << 16)); gr[r1] = temp;}
void Processor::MUL(){
    int32_t temp = gr[r2] * gr[r3]; 
    HI = ((uint32_t)temp)>>16; 
    LOW = temp; 
    gr[r1] = LOW;
}
void Processor::MULU(){uint32_t temp = ugr(r2) * ugr(r3); HI = temp>>16; LOW = temp; gr[r1] = LOW;}
void Processor::DIV(){
    if(!gr[r3]) setStatus(ARITHMETIC_EXCEPTION_BIT); 
    else {
        HI = gr[r2] / gr[r3]; 
        LOW = gr[r2] % gr[r3]; 
        gr[r1] = HI;
    }
}
void Processor::DIVU(){if(!gr[r3]) setStatus(ARITHMETIC_EXCEPTION_BIT); else {HI = ugr(r2) / ugr(r3); LOW = ugr(r2) % ugr(r3); gr[r1] = HI;}}
void Processor::SETL(){gr[r1] = (ugr(r1)&0b0000000011111111) | (instruction<<8);}
void Processor::SETR(){gr[r1] = ((gr[r1]>>8)<<8) | co2;}
void Processor::SET(){gr[r1] = static_cast<int8_t>(instruction&0xFF);}
void Processor::SETU(){gr[r1] = co2;}
void Processor::JMPR(){if(COMPARE_VALUE) nxt_PC = (((int32_t) PC + (((int16_t) (co3 << 4)) >> 4))>>1)<<1;}
void Processor::AND(){gr[r1] = gr[r2] & gr[r3];}
void Processor::OR(){gr[r1] = gr[r2] | gr[r3];}
void Processor::XOR(){gr[r1] = gr[r2] ^ gr[r3];}
void Processor::ADDI(){uint32_t temp = gr[r1] + (int8_t) co2;setStatusTo(0,temp & (1 << 16));gr[r1] = temp;}
void Processor::JMPB(){if(COMPARE_VALUE) nxt_PC = co3 << 4;}

#undef r1
#undef r2
#define r1 ((instruction&0b0000000011110000)>>4)
#define r2 (instruction&0b0000000000001111)

void Processor::ROUTE2(){(this->*instruction_table3[(instruction<<8)>>12])();}
void Processor::MOVS(){gr[r1] = sr[r2&7];}
void Processor::MOV(){gr[r1] = gr[r2];}
void Processor::STAT(){gr[r1] = (STATUS) & (1 << r2);}
void Processor::LOAD(){gr[r1] = *(((int16_t*) memory->data()) + ugr(r2));}
void Processor::STORE(){*(((int16_t*) memory->data()) + ugr(r2)) = gr[r1];}
void Processor::LOADB(){gr[r1] = (*memory)[ugr(r2)];}
void Processor::LOADBU(){gr[r1] = (uint8_t) (*memory)[ugr(r2)];}
void Processor::CMPG(){setStatusTo(COMPARE_BIT, gr[r1] > gr[r2]);}
void Processor::CMPGE(){gr[9] = gr[r1] >= gr[r2];setStatusTo(COMPARE_BIT, gr[r1] >= gr[r2]);}
void Processor::CMPL(){setStatusTo(COMPARE_BIT, gr[r1] < gr[r2]);}
void Processor::CMPLE(){setStatusTo(COMPARE_BIT, gr[r1] <= gr[r2]);}
void Processor::STOREB(){(*memory)[ugr(r2)] = (char) gr[r1];}
void Processor::SFTL(){if(ugr(r2) < 16) gr[r1] <<= gr[r2]; else setStatus(1);}
void Processor::SFTR(){if(ugr(r2) < 16) gr[r1] = ugr(r1) >> r2; else setStatus(1);}
void Processor::SFTRA(){if(ugr(r2) < 16) gr[r1] >>= gr[r2]; else setStatus(1);}

#undef r1
#define r1 (instruction&0b0000000000001111)

void Processor::SADDR(){}
void Processor::JMP(){if(COMPARE_VALUE) nxt_PC = (gr[r1]>>1)<<1;}
void Processor::OUT(){OUT_R = gr[r1]; setStatusTo(OUT_TYPE_BIT, 0); setStatus(OUT_WAIT_BIT);}
void Processor::NOP(){}
void Processor::KILL(){setStatus(5);}
void Processor::NEG(){gr[r1] = -gr[r1];}
void Processor::NOT(){gr[r1] = ~gr[r1];}
void Processor::CMPE(){setStatusTo(COMPARE_BIT, gr[r1] == 0);}
void Processor::CMPNE(){setStatusTo(COMPARE_BIT, gr[r1] != 0);};
void Processor::OUTS(){OUT_R = sr[r1&7]; setStatusTo(OUT_TYPE_BIT, 0); setStatus(OUT_WAIT_BIT);}
void Processor::J() {nxt_PC = (gr[r1]>>1)<<1;}
void Processor::OUTC() {OUT_R = gr[r1]; setStatusTo(OUT_TYPE_BIT, 0); setStatus(OUT_WAIT_BIT); }
void Processor::DEBUG() {setStatus(DEBUG_BIT);}

void Processor::fetch() {
    instruction = *((uint16_t*)(memory->data() + PC));
}

void Processor::execute() {
    nxt_PC = PC + 2;
    (this->*instruction_table1[instruction>>12])();
    PC = nxt_PC;
    CLOCK++;
}

bool Processor::getOut(uint16_t& value, bool& type) {
    if(!(STATUS & (1 << OUT_WAIT_BIT)))
        return false;
    setStatusTo(OUT_WAIT_BIT, 0);
    value = OUT_R;
    type = STATUS & (1 << OUT_TYPE_BIT);
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

bool Processor::shouldDebug() {
    if(!(STATUS & (1 << DEBUG_BIT)))
        return false;
    setStatusTo(DEBUG_BIT, 0);
    return true;
}

Processor::Processor(std::array<char, 641536>* memory) : memory(memory) {
}