#include <bitset>
#include <iostream>

#include "Menu.h"

using namespace std;
using namespace ftxui;

string format_reg(uint16_t value) {
    return bitset<16>(value).to_string().insert(12, " ").insert(8, " ").insert(4, " ");
}

string inst_code(uint16_t inst) {
    return Processor::instruction_codes[(inst>>12) ? (inst>>12) : (inst>>8) ? (inst>>8) + 15 : 31 + (inst>>4)];
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
            out[c * 8 + i] = (s[c] & (1 << (7 - i))) ? '1' : '0';
    }
    return string(out.begin(), out.end());
}

void Menu::convertInputString() {
    inputString = binaryInputMode ? convertNonBinaryString(inputString) : convertBinaryString(inputString);
}

Menu::Menu(deque<char>* queue, Processor* processor, bool step_through) : screen(App::TerminalOutput()), menu(getMenu(processor)), loop(&screen, menu), in_queue(queue), step_through(step_through), processor(processor) {
    lastCycleEval = lastMenuUpdate = chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now().time_since_epoch());
    cyclesPerSecond = -1;
}

bool Menu::hasQuit() {
    return loop.HasQuitted() || quit;
}

void Menu::draw() {
    step_through |= processor->shouldDebug();
    halt = step_through;
    do {
        auto now = chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now().time_since_epoch());
        if((now - lastMenuUpdate).count() > 10000000 && !hasQuit()) {
            lastMenuUpdate = now;
            screen.RequestAnimationFrame();
            loop.RunOnce();
        }
    } while(halt && !hasQuit());
    auto now = chrono::duration_cast<chrono::nanoseconds>(chrono::high_resolution_clock::now().time_since_epoch());
    if(++cntSince == 1000000) {
        cntSince = 0;
        cyclesPerSecond = 1000000000000000LL / max(1LL, (now - lastCycleEval).count());
        lastCycleEval = now;
    }
}

string regStr(uint16_t number) {
    string temp = to_string(number)+")";
    temp.insert(temp.size(), 6 - temp.size(), ' ');
    return format_reg(number)+" ("+temp;
}

Component Menu::getMenu(Processor* proc) {
    InputOption inputDetails = InputOption::Default();

    inputDetails.on_enter = [this] {
        if(binaryInputMode)
            inputString = convertBinaryString(inputString);
        in_queue->insert(in_queue->end(), inputString.begin(), inputString.end());
        inputString.clear();
    };

    Component typing = Input(&inputString, &placeholder, inputDetails);
    typing |= CatchEvent([this] (Event event) {
        return binaryInputMode && (event.is_character() && !(event.character()[0] == '0' || event.character()[0] == '1'));
    });

    advanceButton = Button("Advance",[this] {halt = false;},ButtonOption::Ascii());
    buttons = Container::Horizontal({
        Button("Kill",[this] {quit = true;},ButtonOption::Ascii()),
        Button("Toggle Input",[this] {binaryInputMode = !binaryInputMode;convertInputString();},ButtonOption::Ascii()),
        Button("Reset PC",[=] {proc->sr[0] = 0;},ButtonOption::Ascii()),
    });
    
    Component haltingButton =  Button("Toggle Halt",[this] {
            step_through = !step_through;
            halt = false;
    },ButtonOption::Ascii());
    
    buttons->Add(haltingButton);
    
    allOut = Container::Vertical({
        buttons,
        typing
    });


    // Display would otherwise be selected by default for keyboard navigation
    Component STMode = Renderer(allOut, [this, proc] {
        string prevInst = inst_code(proc->instruction);
        if(step_through)
            buttons->Add(advanceButton);
        else
            advanceButton->Detach();
        return vbox({
            text(". . . . . . . . . . . . . . . . . . . . CustomCPU . . . . . . . . . . . . . . . . . . . ."),
            text("| Registers                                                               INFO"),
            text("| G0 "+regStr(proc->gr[0])+"    G08 "+regStr(proc->gr[8])+"     Cycles/Second : "+(step_through?"-":to_string(cyclesPerSecond))),
            text("| G1 "+regStr(proc->gr[1])+"    G09 "+regStr(proc->gr[9])+"     InputQueue : "+to_string(in_queue->size() / 4)),
            text("| G2 "+regStr(proc->gr[2])+"    G10 "+regStr(proc->gr[10])+"     ERROR : YES"),
            text("| G3 "+regStr(proc->gr[3])+"    G11 "+regStr(proc->gr[11])+"     HALT : "+(step_through?"YES":"NO")),
            text("| G4 "+regStr(proc->gr[4])+"    G12 "+regStr(proc->gr[12])+"     Input : "+(binaryInputMode?"Binary":"Ascii")),
            text("| G5 "+regStr(proc->gr[5])+"    G13 "+regStr(proc->gr[13])),
            text("| G6 "+regStr(proc->gr[6])+"    G14 "+regStr(proc->gr[14])),
            text("| G7 "+regStr(proc->gr[7])+"    G15 "+regStr(proc->gr[15])),
            text("|"),
            text("| PC  "+regStr(proc->sr[0])+"  CLOCK  "+regStr(proc->sr[4])),
            text("| RET "+regStr(proc->sr[1])+"  STATUS "+regStr(proc->sr[5])),
            text("| HI  "+regStr(proc->sr[2])+"  IN     "+regStr(proc->sr[6])),
            text("| LOW "+regStr(proc->sr[3])+"  OUT    "+regStr(proc->sr[7])),
            text("|"),
            hbox({text("| Prev Instruction :"+format_reg(proc->instruction) + " ("), 
                text(prevInst) | color((prevInst.find("JMP") != string::npos) ? ((proc->sr[5] & (1<<6)) ? Color::Green : Color::Red) : Color::White), 
                text(")")}),
            text("| Next Instruction :"+format_reg(proc->memoryAt(proc->sr[0])) + " (" + inst_code(proc->memoryAt(proc->sr[0])) + ")"),
            text("------------------------------------------------------------------------------------------"),
            allOut->Render()
        });
    });


    // Memory Display barely encloses latest memory addr change if it is not already visible

    return STMode;
}