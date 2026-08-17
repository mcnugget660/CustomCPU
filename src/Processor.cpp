#include "Processor.h"

#define r1 ((instruction<<4)>>12)
#define r2 ((instruction<<8)>>12)
#define r3 ((instruction<<12)>>12)

void Processor::ROUTE1(){((*this).*instruction_table1[(instruction<<4)>>12])();}
void Processor::ADD(){gr[r1] = gr[r2] + gr[r3];}
void Processor::MUL(){int32_t temp = ((int32_t) gr[r2]) * gr[r3]; *HI = temp>>16; *LOW = temp; gr[r1] = *LOW;}
void Processor::MULU(){uint32_t temp = ((uint32_t) *ugr[r2]) * *ugr[r3]; *HI = temp>>16; *LOW = temp; *ugr[r1] = *LOW;}
void Processor::DIV(){*HI = gr[r2] / gr[r3]; *LOW = gr[r2] % gr[r3]; gr[r1] = *HI;}
void Processor::DIVU(){*HI = *ugr[r2] / *ugr[r3]; *LOW = *ugr[r2] % *ugr[r3]; *ugr[r1] = *HI;}
void Processor::SETL(){*ugr[r1] = ((*ugr[r1]<<8)>>8) + (instruction<<8);}
void Processor::SETR(){gr[r1] = ((gr[r1]>>8)<<8) + ((instruction<<8)>>8);}
void Processor::SET(){gr[r1] = (int8_t) ((instruction<<8)>>8);}
void Processor::SETU(){gr[r1] = ((instruction<<8)>>8);}
void Processor::JMPC(){nxt_PC = (int16_t) *PC +  (((int16_t)(instruction << 4)) >> 4);}
void Processor::AND(){gr[r1] = gr[r2] & gr[r3];}
void Processor::OR(){gr[r1] = gr[r2] | gr[r3];}
void Processor::XOR(){gr[r1] = gr[r2] ^ gr[r3];}
void Processor::ADDI(){gr[r1] += (((int16_t)(instruction << 8)) >> 8);}
void Processor::ADDIH(){gr[r1] += (((int16_t)(instruction << 8)));}

#undef r1
#undef r2
#define r1 ((instruction<<8)>>12)
#define r2 ((instruction<<12)>>12)

void Processor::ROUTE2(){((*this).*instruction_table1[(instruction<<8)>>12])();}
void Processor::MOVS(){}
void Processor::MOV(){}
void Processor::STAT(){}
void Processor::LOAD(){}
void Processor::STORE(){}
void Processor::JMPE(){}
void Processor::JMPNE(){}
void Processor::JMPG(){}
void Processor::JMPGE(){}
void Processor::SFTLC(){}
void Processor::SFTRC(){}
void Processor::SFTRAC(){}
void Processor::SFTL(){}
void Processor::SFTR(){}
void Processor::SFTRA(){}

#undef r1
#define r1 ((instruction<<12)>>12)

void Processor::RET(){}
void Processor::JMP(){}
void Processor::OUT(){}
void Processor::NOP(){}
void Processor::KILL(){}
void Processor::FORK(){}
void Processor::INV(){}
void Processor::COMP(){}
void Processor::CEIL(){}



void Processor::fetch() {
    instruction = memory.at(*PC);
}

void Processor::execute() {
    ((*this).*instruction_table1[instruction>>12])();
}

Processor::Processor(std::array<int16_t, 641536> memory) : 
    memory(memory), instruction_table1{
        ROUTE1,
        ADD,
        MUL,
        MULU,
        DIV,
        DIVU,
        SETL,
        SETR,
        SET,
        SETU,
        JMPC,
        AND,
        OR,
        XOR,
        ADDI,
        ADDIH
    }, instruction_table2{
        ROUTE2,
        MOVS,
        MOV,
        STAT,
        LOAD,
        STORE,
        JMPE,
        JMPNE,
        JMPG,
        JMPGE,
        SFTLC,
        SFTRC,
        SFTRAC,
        SFTL,
        SFTR,
        SFTRA
    }, instruction_table3{
        RET,
        JMP,
        OUT,
        NOP,
        KILL,
        FORK,
        INV,
        COMP,
        CEIL
    } {
}