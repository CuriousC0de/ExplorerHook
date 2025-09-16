#include "includes.h"

void CreateNewConsole()
{
    AllocConsole();
    FILE* newConsole;
    freopen_s(&newConsole, "CONOUT$", "w", stdout);
}

LPWSTR CreateSpaceString(LPWSTR input) {
    // Calculate the size of the input string
    size_t size = wcslen(input); // Get the number of wide characters
    // Allocate memory for the new LPWSTR (size + 1 for null terminator)
    LPWSTR spaceString = new wchar_t[size + 1];
    // Fill the new string with spaces
    for (size_t i = 0; i < size; ++i) {
        spaceString[i] = L' '; // Fill with space characters
    }
    spaceString[size] = L'\0'; // Null-terminate the string
    return spaceString; // Return the new LPWSTR
}

const wchar_t* activateWindows = L"Activate Windows";
const wchar_t* settingsWindows = L"Go to Settings to activate Windows.";


typedef int (WINAPI* DrawTextExW_t)(HDC, LPWSTR, int, LPRECT, UINT, LPDRAWTEXTPARAMS);
DrawTextExW_t originalDrawTextExW = nullptr;

int WINAPI HookedDrawTextExW(HDC hdc, LPWSTR lpchText, int cchText, LPRECT lprc, UINT uFormat, LPDRAWTEXTPARAMS lpdtp) 
{
    if (lpchText != NULL)
    {
        if (wcscmp(lpchText, activateWindows) == 0 || wcscmp(lpchText, settingsWindows) == 0)
        {
            return originalDrawTextExW(hdc, CreateSpaceString(lpchText), cchText, lprc, uFormat, lpdtp);
        }
    }
    return originalDrawTextExW(hdc, lpchText, cchText, lprc, uFormat, lpdtp);
}

void init()
{
    //CreateNewConsole();

    if (MH_Initialize() != MH_OK)
    {
        printf("\nMH_Initialize != MH_OK");
        return;
    }
    if (MH_CreateHook(&DrawTextExW, &HookedDrawTextExW, reinterpret_cast<LPVOID*>(&originalDrawTextExW)) != MH_OK || MH_EnableHook(&DrawTextExW) != MH_OK) {
        printf("\nMH_CreateHook failed");
        return;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) { switch (ul_reason_for_call) { case DLL_PROCESS_ATTACH: CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)init, 0, 0, NULL); } return TRUE; }