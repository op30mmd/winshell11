#include <windows.h>
#include <string>
#include <iostream>

int main() {
    std::cout << "Watchdog started." << std::endl;

    while (true) {
        Sleep(10000);
    }

    return 0;
}
