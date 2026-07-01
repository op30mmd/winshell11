#include <iostream>
#include <string>
#include <windows.h>

int main() {
    std::cout << "Watchdog started." << std::endl;

    while (true) {
        Sleep(10000);
    }

    return 0;
}
