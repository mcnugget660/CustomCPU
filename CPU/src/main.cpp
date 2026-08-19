#include <iostream>
#include <vector>
#include <Processor.h>
#include <fstream>
#include <thread>
#include <string>

#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/loop.hpp"
#include "ftxui/dom/elements.hpp"
#include <csignal>
#include <bitset>

using namespace std;
using namespace ftxui;

int loadMemory();
void configureProgram();
Component getMenu(App& screen);

array<int16_t, 641536> memory;
vector<Processor> processes;
deque<char> in_queue;

unsigned long long currentSelection = 0;
unsigned long long cyclesPerSecond = 0;
bool step_through = false;
bool halt = false;

int main() {
    if(loadMemory()) return 0;
    configureProgram();
    processes.emplace_back(&memory);

    auto screen = App::TerminalOutput();
    auto menu = getMenu(screen);
    Loop loop(&screen, menu);
    loop.RunOnce();

    chrono::nanoseconds lastMenuUpdate = chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now().time_since_epoch());
    chrono::nanoseconds lastCycleEval = lastMenuUpdate;
    chrono::nanoseconds now;

    int cntSince = 0;
    uint16_t OUT_VALUE;
    ofstream output("output.txt", ios_base::binary);

    while(processes.size() && !loop.HasQuitted()) {
        // Menu Updates
        halt = step_through;
        do {
            now = chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now().time_since_epoch());
            if((now - lastMenuUpdate).count() > 10000000 && processes.size()) {
                lastMenuUpdate = now;
                currentSelection = min(currentSelection, processes.size() - 1);
                screen.RequestAnimationFrame();
                loop.RunOnce();
            }
        } while(halt && !loop.HasQuitted() && processes.size());

        // Processor Cycle
        if(in_queue.size()){
            bool success = false;
            for(Processor& p : processes) success |= p.setIn(in_queue.front());
            if(success) in_queue.pop_front();
        }
         
        for(Processor& p : processes) p.fetch();
        for(int i = 0; i < processes.size(); i++) {
            processes[i].execute();
            if(processes[i].getOut(&OUT_VALUE))
                output.write((char*)&OUT_VALUE, 2);

            if(processes[i].shouldKill()) {
                processes.erase(processes.begin() + i);
                i--;
            }
        }
        for(Processor&p : processes) {
            if(p.shouldFork()) {
                processes.push_back(p);
                processes[processes.size() - 1].setUniverse(p.getUniverse() + 1);
            }
        }

        now = chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now().time_since_epoch());
        if(++cntSince == 1000000) {
            cntSince = 0;
            cyclesPerSecond = 1000000000000000LL / max(1LL, (now - lastCycleEval).count());
            lastCycleEval = now;
        }
    }
    output.close();
    cout << endl << "\nProgram Ended" << endl;
    // Presumably an onexit function outputs the final newline
    return 0;
}

int loadMemory() {
    ifstream memory_file("memory.txt");
    if(!memory_file.is_open()) {
        cout << "Failed to open memory.txt" << endl;
        return 1;
    }

    string ln;
    int index = 0;
    while(getline(memory_file, ln) && index < memory.size())
        memory[index++] = stoi(ln, nullptr, 2);

    memory_file.close();
    return 0;
}

void configureProgram() {
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

string inputString = "";
string placeholder = "...";
int binaryInputMode;

string display_reg(int16_t hw) {
    return bitset<16>(hw).to_string().insert(12, " ").insert(8, " ").insert(4, " ");
}

string inst_code(int16_t inst) {
    return Processor::instruction_codes[(inst>>12) ? (inst>>12) : ((inst<<4)>>12) ? ((inst<<4)>>12) + 16 : 31 + ((inst<<8)>>12)];
}

string convertBinaryString(string s) {
    vector<char> out((s.size() + 7) / 8);
    for(int i = 0; i < s.size(); i += 8) {
        char c = 0;
        for(int y = 0; y < 8 && i + y < s.size(); y++)
            c += (1 << (7 - y)) * (s[i + y] == '1');
        out[i/8] = c;
    }
    return string(out.begin(), out.end());
}

string convertNonBinaryString(string s) {
    vector<char> out(s.size() * 8);
    for(int c = 0; c < s.size(); c++) {
        for(int i = 0; i < 8; i++)
            out[c * 8+ i] = c & (1 << (7 - i));
    }
    return string(out.begin(), out.end());
}

void convertInputString() {
    inputString = binaryInputMode ? convertNonBinaryString(inputString) : convertBinaryString(inputString);
}

Component allOut; //Not part of menu heirarcy so must remain in scope

Component getMenu(App& screen) {
    
    Component typing = Input(&inputString, placeholder);

    typing |= CatchEvent([&] (Event event) {
        if(event == Event::Special("\n")) {
            if(binaryInputMode)
                inputString = convertBinaryString(inputString);
            in_queue.insert(in_queue.end(), inputString.begin(), inputString.end());
            inputString.clear();
            return true;
        }
        return (binaryInputMode && !(event == Event::Special("0") || event == Event::Special("1")));
    });


    Component buttons = Container::Horizontal({
        Button("Exit",[&] {processes.clear();},ButtonOption::Ascii()),
        Button("Kill",[&] {processes.erase(processes.begin() + currentSelection);},ButtonOption::Ascii()),
        Button("Toggle Input",[&] {binaryInputMode = !binaryInputMode;convertInputString();},ButtonOption::Ascii()),
        Button("<---",[&] {if(currentSelection) currentSelection--;},ButtonOption::Ascii()),
        Button("--->",[&] {if(currentSelection<processes.size()-1) currentSelection++;},ButtonOption::Ascii()),
        Button("Toggle Halt",[&] {step_through = !step_through;halt = false;},ButtonOption::Ascii()),
        Button("Advance",[&] {halt = false;},ButtonOption::Ascii())
    });

    allOut = Container::Vertical({
        buttons,
        typing
    });

    Component STMode = Renderer(allOut, [&] {
        return vbox({
            text(". . . . . . . . . . . . . . CustomCPU . . . . . . . . . . . . . ."),
            text("| Registers                                             INFO"),
            text("| G0 "+display_reg(processes[currentSelection].gr[0])+"    G08 "+display_reg(processes[currentSelection].gr[8])+"     PC : "+to_string(processes[currentSelection].sr[0])),
            text("| G1 "+display_reg(processes[currentSelection].gr[1])+"    G09 "+display_reg(processes[currentSelection].gr[9])+"     CLK : "+to_string(processes[currentSelection].sr[4])),
            text("| G2 "+display_reg(processes[currentSelection].gr[2])+"    G10 "+display_reg(processes[currentSelection].gr[10])+"     ERROR : YES"),
            text("| G3 "+display_reg(processes[currentSelection].gr[3])+"    G10 "+display_reg(processes[currentSelection].gr[11])+"     HALT : "+(step_through?"YES":"NO")),
            text("| G4 "+display_reg(processes[currentSelection].gr[4])+"    G10 "+display_reg(processes[currentSelection].gr[12])+"     Input : "+(binaryInputMode?"Binary":"Ascii")),
            text("| G5 "+display_reg(processes[currentSelection].gr[5])+"    G10 "+display_reg(processes[currentSelection].gr[13])+"     CPU : "+to_string(currentSelection)+"/"+to_string(processes.size())),
            text("| G6 "+display_reg(processes[currentSelection].gr[6])+"    G10 "+display_reg(processes[currentSelection].gr[14])+"     Cycles/Second : "+(step_through?"-":to_string(cyclesPerSecond))),
            text("| G7 "+display_reg(processes[currentSelection].gr[7])+"    G10 "+display_reg(processes[currentSelection].gr[15])+"     InputQueue : "+to_string(in_queue.size() / 4)),
            text("|"),
            text("| PC  "+display_reg(processes[currentSelection].sr[0])+"  CLOCK  "+display_reg(processes[currentSelection].sr[4])),
            text("| RET "+display_reg(processes[currentSelection].sr[1])+"  STATUS "+display_reg(processes[currentSelection].sr[5])),
            text("| HI  "+display_reg(processes[currentSelection].sr[2])+"  IN     "+display_reg(processes[currentSelection].sr[6])),
            text("| LOW "+display_reg(processes[currentSelection].sr[3])+"  OUT    "+display_reg(processes[currentSelection].sr[7])),
            text("|"),
            text("| Prev Instruction :"+display_reg(processes[currentSelection].instruction)+" ("+inst_code(processes[currentSelection].instruction)+")"),
            text("| Next Instruction :"+display_reg(memory[processes[currentSelection].sr[0]])+" ("+inst_code(memory[processes[currentSelection].sr[0]])+")"),
            allOut->Render()
        });
    });

    return STMode;
}