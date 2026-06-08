#include "farm/ui/Win32FarmApp.h"

#ifdef _WIN32
#include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return farm::ui::RunFarmApp();
}
#else
int main() { return 0; }
#endif
