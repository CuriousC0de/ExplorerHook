#include "includes.h"

bool FileExists(const std::string& filePath) {
    DWORD fileAttributes = GetFileAttributes(filePath.c_str());
    return (fileAttributes != INVALID_FILE_ATTRIBUTES && !(fileAttributes & FILE_ATTRIBUTE_DIRECTORY));
}

DWORD FindProcessIdByName(const std::string& processName) {
    // Convert std::string to LPCSTR
    const char* processNameCStr = processName.c_str();

    // Create a snapshot of all processes
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return 0; // Failed to create snapshot
    }

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    // Iterate through the processes
    if (Process32First(hSnapshot, &processEntry)) {
        do {
            // Compare the process name with the given name
            if (strcmp(processEntry.szExeFile, processNameCStr) == 0) {
                CloseHandle(hSnapshot);
                return processEntry.th32ProcessID; // Return the PID
            }
        } while (Process32Next(hSnapshot, &processEntry));
    }
    CloseHandle(hSnapshot);
    return 0; // Process not found
}

bool LoadLibraryRemote(DWORD processId, const std::string& dll) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        std::cerr << "Failed to open process: " << GetLastError() << std::endl;
        return false;
    }

    // Allocate memory in the remote process for the DLL path
    LPVOID pDllPath = VirtualAllocEx(hProcess, NULL, dll.size() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pDllPath) {
        std::cerr << "Failed to allocate memory in remote process: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    // Write the DLL path to the allocated memory
    if (!WriteProcessMemory(hProcess, pDllPath, dll.c_str(), dll.size() + 1, NULL)) {
        std::cerr << "Failed to write memory in remote process: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Get the address of LoadLibraryA
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (hKernel32 == 0) return false;

    FARPROC pLoadLibraryA = GetProcAddress(hKernel32, "LoadLibraryA");

    // Create a remote thread that calls LoadLibraryA with the DLL path
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryA, pDllPath, 0, NULL);
    if (!hThread) {
        std::cerr << "Failed to create remote thread: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    // Wait for the thread to finish and clean up
    WaitForSingleObject(hThread, INFINITE);
    VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    return true;
}



void main()
{
    std::string hook_dll = "ExplorerHook\\x64\\Release\\ExplorerHook.dll";

    if (!FileExists(hook_dll))
    {
        printf("\nmissing ExplorerHook.dll");
        return;
    }

    system("taskkill /f /im explorer.exe");
    Sleep(1000);

    PROCESS_INFORMATION processInfo;
    STARTUPINFO startupInfo;
    ZeroMemory(&processInfo, sizeof(processInfo));
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    if (CreateProcess(
        "c:\\windows\\explorer.exe",
        NULL,
        NULL,
        NULL,
        FALSE,
        CREATE_SUSPENDED, // suspended
        NULL,
        NULL,
        &startupInfo,
        &processInfo
    )) {
        std::cout << "Process created successfully. Process ID: " << processInfo.dwProcessId << std::endl;
    }

    if (!LoadLibraryRemote(processInfo.dwProcessId, hook_dll))
    {
        MessageBoxA(0, "failed", "", 0);
    }

    ResumeThread(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
}

