#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_executions = 1;

static const char g_banner[] = "Hollowed payload executed";

int main() {
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    char comp[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD compLen = sizeof(comp);
    GetComputerNameA(comp, &compLen);

    char mod[MAX_PATH];
    GetModuleFileNameA(NULL, mod, MAX_PATH);

    char desktop[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktop) != S_OK)
        strcpy(desktop, "C:\\");

    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "%s\\payload_dump.txt", desktop);

    char *report = (char *)malloc(2048);
    if (!report)
        return 1;
    report[0] = '\0';

    strcat(report, g_banner);
    strcat(report, "\r\n");
    sprintf(report + strlen(report),
        "Computer   : %s\r\n"
        "Module     : %s\r\n"
        "Processors : %lu\r\n"
        "PageSize   : %lu\r\n"
        "TickCount  : %lu\r\n"
        "Executions : %d\r\n",
        comp, mod, si.dwNumberOfProcessors, si.dwPageSize,
        GetTickCount(), g_executions);

    char *note = (char *)malloc(strlen("CRT malloc/free OK") + 1);
    strcpy(note, "CRT malloc/free OK");
    strcat(report, note);
    strcat(report, "\r\n");
    free(note);

    HANDLE hFile = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        WriteFile(hFile, report, (DWORD)strlen(report), &written, NULL);
        CloseHandle(hFile);
    }

    FILE *fp = fopen(path, "ab");
    if (fp) {
        fputs("[CRT fopen/fputs OK]\r\n", fp);
        fclose(fp);
    }

    MessageBoxA(NULL, report, "Payload", MB_OK | MB_ICONINFORMATION);

    free(report);
    return 0;
}
