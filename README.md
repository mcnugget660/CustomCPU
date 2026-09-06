# CustomCPU and Assembler

## Table of Contents
+ [About](#about)
+ [Installation](#Installation)
+ [CPU Usage](#cpuusage)
+ [Assembler Usage](#assemblerusage)

## About <a name = "about"></a>
This project implements a CPU simulator and assembler following a custom 16-bit ISA and assembler specification. The simulator includes a menu and various input/output features for convience, while the assembler aims to provide comprehensive error messages and warnings.  
  
The purpose of this project is to experiement with the inclusion of specific CPU instructions and assembler features in a practical and iterative process inorder to learn more about low-level languages and computing concepts.
  

# Installation <a name = "Installation"></a>

Download the repository and install using cmake

```
git clone github.com/mcnugget660/CustomCPU
cmake --build build
```

  

# CPU Usage <a name = "cpuusage"></a>

The CPU program takes in a memory file and optionaly an input file. There is also the choice to start the program in stepthrough mode, which requires the user to manually step through each instruction.
  
Input, either from a file or entered from the menu, is queued and processed a halfword at a time by the specified program. Output is directly writen to the file output.txt and will override itself each run.

## Flags

Flags allow you to specify startup parameters without the initial terminal input

#### --input=false
Disables input entirely

#### --stepthrough=true  
Enables stepthrough mode on startup

#### --stepthrough=true  
Disables stepthrough mode on startup

#### --memorymode=binary  
Specifies that the memory file is in a binary format

#### --memorymode=text  
Specifies that the memory file reads one halfword per line where each bit is either an ASCII 0 or 1

#### -if filename  
Uses filename as the input file

#### -mf filename  
Uses filename as the memory file

  

# Assembler Usage <a name = "cpuusage"></a>

The Assembler will produce an output file named filename.asm based on the specification listed in [Assembly.txt](doc/Assembly.txt). You should also be familiar with the general CPU architecture as specified in [Specification.txt](doc/Specification.txt).
  
To run this file, enclude it as the memory file when running the CPU.
```
Assembler.exe filename
```

## Flags

#### --littleEndian  
Ensures machine code is oriented in littleEndian

#### --bigEndian  
Ensres machine code is oriented in bigEndian

#### -o offset  
Adds offset to all relative addresses