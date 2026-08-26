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

int loadMemory(array<uint16_t, 641536>& memory);
void configureProgram(deque<char>& in_queue, bool& step_through);

int main() {
    // Don't store on stack
    static std::array<uint16_t, 641536> memory;
    deque<char> in_queue;
    bool step_mode = false;

    if(loadMemory(memory)) return 1;
    configureProgram(in_queue, step_mode);

    Processor processor(&memory);
    Menu menu(&in_queue, &processor, step_mode);

    uint16_t OUT_VALUE;
    ofstream output("output.txt", ios_base::binary);

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

    output.close();
    cout << endl << "\nProgram Ended" << endl;
    // Presumably an onexit function outputs the final newline
    return 0;
}

int loadMemory(array<uint16_t, 641536>& memory) {
    cout << "Memory file name: ";
    string name, type;
    cin >> name;
    cout << "Is the file plaintext? (y/n) (default binary) : ";
    cin >> type;
    bool binary = !(type == "y" || type == "Y");

    ifstream memory_file(name, binary ? ios_base::binary : ios_base::out);
    if(!memory_file.is_open()) {
        cout << "Failed to open " << name << endl;
        return 1;
    }

    if(binary) { 
        size_t numBytes = filesystem::file_size(name);
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

void configureProgram(deque<char>& in_queue, bool& step_through) {
    cout << "Enable File Input? (y/n) : ";
    string ans;
    cin >> ans;
    if(ans == "y" || ans == "Y") {
        cout << "File name : ";
        cin >> ans;
        ifstream inFile = ifstream(ans, ios_base::binary);
        if(!inFile.is_open())
            cout << "Invalid File\n";
        else {
            in_queue = deque<char>(istreambuf_iterator<char>(inFile), istreambuf_iterator<char>());
            if(in_queue.size() & 1) in_queue.push_back(0);
        }
        inFile.close();
    }

    cout << "Enable Step-Through Mode? (y/n) : ";
    cin >> ans;
    if(ans == "y" || ans == "Y")
        step_through = true;
}



