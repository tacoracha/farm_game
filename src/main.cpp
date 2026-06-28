#ifdef _WIN32
#include "farm/ui/Win32FarmApp.h"

#include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return farm::ui::RunFarmApp();
}
#elif defined(__APPLE__)
#include "farm/ui/MacFarmApp.h"

int main() {
    return farm::ui::RunMacFarmApp();
}
#else
int main() { return 0; }
#endif
