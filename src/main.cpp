#include <iostream>
#include <vector>
#include <Processor.h>
#include <fstream>
#include <thread>
#include <string>

#include "ftxui/component/app.hpp"
#include "ftxui/component/captured_mouse.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/loop.hpp"
#include <csignal>

using namespace std;
using namespace ftxui;

int loadMemory();
void configureProgram();
Component getMenu(App& screen);

array<int16_t, 641536> memory;
vector<Processor> processes;

uint16_t IN, OUT;
bool in_ready = false;
bool out_ready = false;
vector<char> in_queue;

bool step_through = false;

int main() {
    if(loadMemory()) return 0;
    configureProgram();

    static App screen = App::TerminalOutput();
    auto menu = getMenu(screen);
    Loop loop(&screen, menu);
    auto last = std::chrono::duration_cast<std::chrono::milliseconds>(chrono::system_clock::now().time_since_epoch());
    
    std::signal(SIGINT, [](int) {
        screen.Exit();
    });
    std::signal(SIGTERM, [](int) {
        screen.Exit();
    });

    // Begin
    processes.emplace_back(&memory);
    loop.RunOnce();
   
    while(processes.size()) {
        if(in_ready){
            in_ready = false;
            for(Processor& p : processes) p.setIn(IN);
        }
         
        for(Processor& p : processes) p.fetch();
        for(int i = 0; i < processes.size(); i++) {
            processes[i].execute();
            out_ready |= processes[i].setOut(&OUT);
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
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(chrono::system_clock::now().time_since_epoch());
        if((now - last).count() > 10) {
            last = now;
            loop.RunOnce();
        }
    }

    screen.Exit();
    cout << "Program Ended" << endl;

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
    ifstream inFile;

    cout << "Enable Text Input? (y/n) : ";
    string ans;
    cin >> ans;
    if(ans == "y" || ans == "Y") {
        cout << "File name : ";
        cin >> ans;
        inFile = ifstream(ans, ios_base::binary);
        if(!inFile.is_open())
            cout << "Invalid File\n";
        else {
            in_queue = vector<char>(std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>());
            if(in_queue.size() & 1) in_queue.push_back(0);
        }
    }

    cout << "Enable Step-Through Mode? (y/n) : ";
    cin >> ans;
    if(ans == "y" || ans == "Y")
        step_through = true;

    inFile.close();
}

bool killSwitch = false;
bool restartSwitch = false;
string inputString = "";
string placeholder = "...";
unsigned long long currentSelection = 0;

Component getMenu(App& screen) {
    
    Component typing = Input({
        .content = &inputString,
        .placeholder = &placeholder,
    });

    auto buttons = Container::Horizontal({
        Button("Exit",[&] { processes.clear();  screen.RequestAnimationFrame();},ButtonOption::Animated()),
        Button("Kill",[&] { 
            currentSelection = std::min(currentSelection, processes.size() - 1);
            if(processes.size()) processes.erase(processes.begin() + currentSelection);
             screen.RequestAnimationFrame();
        },ButtonOption::Animated()),
        Button("Step Through",[&] { step_through = !step_through; screen.RequestAnimationFrame();},ButtonOption::Animated())
    });

    auto input = Container::Vertical({
        buttons,
        typing
    });

    auto basic = Renderer([&] {
        return text(". . . . . . . . . . . . . . CustomCPU . . . . . . . . . . . . . .");
    });
    auto STMode = Renderer([&] {
        return text(". . . . . . . . . . . . . . CustomCPU . . . . . . . . . . . . . .");
    });

    auto menu = Renderer([&] {
        return step_through ? STMode->Render() : basic->Render();
    });

    return input;
}