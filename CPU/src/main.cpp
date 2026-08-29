#include <iostream>
#include <vector>
#include <fstream>
#include <thread>
#include <string>
#include <deque>
#include <cstdint>
#include <csignal>
#include <bitset>
#include <filesystem>
#include <array>

#include <Menu.h>

using namespace std;

int configureProgram(int argc, char* argv[], deque<char>& in_queue, bool& step_through, array<uint16_t, 641536>& memory);

int main(int argc, char* argv[]) {
    deque<char> in_queue;
    bool step_mode = false;
    static std::array<uint16_t, 641536> memory; // Don't store on stack
    if(configureProgram(argc, argv, in_queue, step_mode, memory)) return 1;

    Processor processor(&memory);
    Menu menu(&in_queue, &processor, step_mode);

    uint16_t OUT_VALUE;
    ofstream output("output.txt", ios_base::binary);
    output.is_open();

    while(!menu.hasQuit()) {
        menu.draw();
        if(menu.hasQuit()) break;

        // Processor Cycle
        if(in_queue.size()){
            bool success = processor.setIn(in_queue.front());
            if(success) in_queue.pop_front();
        }
         
        processor.fetch();
        processor.execute();

        if(processor.getOut(&OUT_VALUE))
            output.write((char*)&OUT_VALUE, 2);

        if(processor.shouldKill()) break;
    }

    output.flush();
    output.close();
    cout << endl << "\nProgram Ended" << endl;
    // Presumably an onexit function outputs the final newline
    return 0;
}

int configureProgram(int argc, char* argv[], deque<char>& in_queue, bool& step_through, array<uint16_t, 641536>& memory) {
    string ans = "no", fileName, memName;
    bool setFileEnable = false, fileSet = false, stepSet = false, memSet = false, modeSet = false;
    bool binaryMode;

    for(int i = 1; i < argc; i++) {
        if(string(argv[i]) == "--input=false")
            setFileEnable = true;
        else if(string(argv[i]) == "--stepthrough=true")
            stepSet = step_through = true;
        else if(string(argv[i]) == "--stepthrough=false")
            stepSet = !(step_through = false);
        else if(string(argv[i]) == "--memorymode=binary" || string(argv[i]) == "--memorymode=b")
            modeSet = binaryMode = true;
        else if(string(argv[i]) == "--memorymode=text" || string(argv[i]) == "--memorymode=t")
            modeSet = !(binaryMode = false);
        else if(string(argv[i]) == "-if") {
            if(i == argc - 1) {
                cout << "Error: no file name provided for flag -if" << endl;
                return 1;
            }
            fileName = argv[++i];
            fileSet = true;
        } else if(string(argv[i]) == "-mf") {
            if(i == argc - 1) {
                cout << "Error: no file name provided for flag -mf" << endl;
                return 1;
            }
            memName = argv[++i];
            memSet = true;
        } else
            cout << "Warning: unrecognized argument \"" << argv[i] << "\"" << endl;
    }

    if(setFileEnable && fileSet)
        cout << "Warning: Arguments are nonsensical: disable input + set input file (will read file)" << endl;

    if(!setFileEnable) {
        cout << "Enable File Input? (y/n) : ";
        cin >> ans;
    }
    if(fileSet || ans == "y" || ans == "Y") {
        if(!fileSet) {
            cout << "File name : ";
            cin >> fileName;
        }
        ifstream inFile = ifstream(fileName, ios_base::binary);
        if(!inFile.is_open())
            cout << "Invalid File\n";
        else {
            in_queue = deque<char>(istreambuf_iterator<char>(inFile), istreambuf_iterator<char>());
            if(in_queue.size() & 1) in_queue.push_back(0);
        }
        inFile.close();
    }

    if(!stepSet) {
        cout << "Enable Step-Through Mode? (y/n) : ";
        cin >> ans;
        step_through = (ans == "y" || ans == "Y");
    }

    // Memory
    if(!memSet) {
        cout << "Memory file name: ";
        cin >> memName;
    }
    if(!modeSet) {
        cout << "Is the memory file plaintext? (y/n) (default binary) : ";
        cin >> ans;
        binaryMode = !(ans == "y" || ans == "Y");
    }

    ifstream memory_file(memName, binaryMode ? ios_base::binary : ios_base::out);
    if(!memory_file.is_open()) {
        cout << "Failed to open " << memName << endl;
        return 1;
    }

    if(binaryMode) { 
        size_t numBytes = filesystem::file_size(memName);
        long long index = 0;
        while(index / 2 < memory.size() && index < numBytes) {
            memory_file.read(((char*) memory.data()) + index, 1);
            index++;
        }
    } else {
        string ln;
        int index = 0;
        while(getline(memory_file, ln) && index < memory.size())
            memory[index++] = stoi(ln, nullptr, 2);
    }

    memory_file.close();
    return 0;
}



