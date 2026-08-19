#include <iostream>
#include <string>
#include <fstream>

using namespace std;

int main(int argc, char* argv[]) {
    if(argc <= 0) {
        cout << "Error: No File" << endl;
        return 1;
    }
    ifstream file{string(argv[0])}; // vexing
    if(!file.is_open()) {
        
    }

    return 0;
}