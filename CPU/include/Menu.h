#include <Processor.h>

#include "ftxui/component/app.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/loop.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/component/component_options.hpp"

class Menu {
public:
    Menu(std::deque<char>* queue, Processor* processor, bool step_through);
    void draw();
    bool hasQuit();
private:

    // Not part of menu heirarcy so must remain in scope
    ftxui::Component allOut = nullptr; // ORDER DEPENDENT
    ftxui::Component advanceButton = nullptr; // ORDER DEPENDENT
    ftxui::Component buttons = nullptr;

    ftxui::App screen; // ORDER DEPENDENT
    ftxui::Component menu; // ORDER DEPENDENT
    ftxui::Loop loop; // ORDER DEPENDENT


    std::deque<char>* in_queue = nullptr;
    std::chrono::nanoseconds lastMenuUpdate;
    std::chrono::nanoseconds lastCycleEval;

    // Options
    int binaryInputMode = false;
    std::string inputString = "";
    std::string placeholder = "...";

    long long cyclesPerSecond = 0;
    long long cntSince = 0;
    bool step_through = false;
    bool halt = false;
    bool quit = false;

    Processor* processor;

    void convertInputString();
    ftxui::Component getMenu(Processor* processor);
};