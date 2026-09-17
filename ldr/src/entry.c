#include "../includes/core/core.h"




BOOL APIENTRY DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved) {
    if (dwReason == DLL_PROCESS_ATTACH)
       LdrMain();
    return TRUE;
}
