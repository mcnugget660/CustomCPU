Usage

Assemble Program
./Assembler.exe program_name

Quick Run program
./CPU --stepthrough=true --input=false -mf program_name.asm --memorymode=binary


# Not all commands are guaranteed to work right now

General Information

16-bit single core CPU

641536 x 16-bit memory

For ease of use, a direct input-output system is built into the CPU itself
Input is queued and fed into the IN register whenever the CPU reads from IN
OUT is immediately written to a file

Registers (25)
 - General purpose registers (0-15)
 - PC register (S0)
 - Return register (S1)
 - HI (S2)
 - LOW (S3)
 - CLOCK (S4)
 - STATUS (S5)
 - IN (S6)
 - OUT (S7)
 - Instruction
