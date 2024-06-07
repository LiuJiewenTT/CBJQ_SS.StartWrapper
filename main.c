#define UNICODE
#define _UNICODE

#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include "utils.h"


#define TEMPSTR_LENGTH 2048
#define TEMPWSTR_LENGTH 2048


const int check_interval1=100;
const int check_interval1_maxcnt=10;
const int check_interval2=1000;


// 函数声明
BOOL IsProcessElevated(DWORD processId);
BOOL IsProcessRunning(HANDLE hProcess);

int main(int argc, char **argv) {
    if( argc < 2 ){
        return EXIT_FAILURE;
    }
    wchar_t *pw1 = NULL;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // 设置要启动的子进程路径，注意替换为实际要启动的程序路径
    wchar_t szCmdline[TEMPWSTR_LENGTH];
    pw1 = WCharChar(argv[1]);
    wcsncpy(szCmdline, pw1, TEMPWSTR_LENGTH);
    free2NULL(pw1);
    szCmdline[TEMPWSTR_LENGTH-1] = 0;

    // 启动子进程
    if (!CreateProcess(
        NULL,       // No module name (use command line)
        szCmdline,  // Command line
        NULL,       // Process handle not inheritable
        NULL,       // Thread handle not inheritable
        FALSE,      // Set handle inheritance to FALSE
        0,          // No creation flags
        NULL,       // Use parent's environment block
        NULL,       // Use parent's starting directory 
        &si,        // Pointer to STARTUPINFO structure
        &pi)        // Pointer to PROCESS_INFORMATION structure
    ) {
        printf("CreateProcess failed (%d).\n", GetLastError());
        return EXIT_FAILURE;
    }

    printf("Started child process with PID: %lu\n", pi.dwProcessId);

    int exit_value = EXIT_SUCCESS;
    char tempstr1[TEMPSTR_LENGTH];
    sprintf(tempstr1, "%s.test", argv[0]);

    if( !file_exists(tempstr1) ){
        // 检查子进程是否以高权限运行
        int check_stage=1;
        int check_cnt=0;
        int exit_status=0;
        int elevated=0;
        do{
            if (IsProcessElevated(pi.dwProcessId)) {
                printf("The child process is running with elevated privileges.\n");
                elevated = 1;
                break;
            } else {
                if( !check_cnt ){
                    printf("The child process is not running with elevated privileges.\n");
                }
                exit_value = EXIT_FAILURE;
            }
            if( check_stage == 1 ){
                ++check_cnt;
                Sleep(check_interval1);
                if( check_cnt == check_interval1_maxcnt ){
                    check_stage = 2;
                }
            }
            else {
                Sleep(check_interval2);
            }
            
        }while(exit_status=IsProcessRunning(pi.hProcess));

        if(!elevated && !exit_status){
            printf("The child process was not elevated and does not exist anymore.\n");
            exit_value = EXIT_FAILURE;
        }
    }
    else {
        printf("Found test flag file: %s\n", tempstr1);
    }

    // // 等待子进程结束
    // WaitForSingleObject(pi.hProcess, INFINITE);

    // 关闭进程和线程句柄
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exit_value;
}

// 检查指定进程是否以管理员权限运行
BOOL IsProcessElevated(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (hProcess == NULL) {
        printf("OpenProcess failed (%d).\n", GetLastError());
        return FALSE;
    }

    HANDLE hToken = NULL;
    if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
        printf("OpenProcessToken failed (%d).\n", GetLastError());
        CloseHandle(hProcess);
        return FALSE;
    }

    TOKEN_ELEVATION elevation;
    DWORD dwSize;
    if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize)) {
        CloseHandle(hToken);
        CloseHandle(hProcess);
        return elevation.TokenIsElevated;
    } else {
        printf("GetTokenInformation failed (%d).\n", GetLastError());
        CloseHandle(hToken);
        CloseHandle(hProcess);
        return FALSE;
    }
}


BOOL IsProcessRunning(HANDLE hProcess) {
    DWORD exitCode;
    if (GetExitCodeProcess(hProcess, &exitCode)) {
        // 如果进程还在运行，exitCode 会是 STILL_ACTIVE
        return exitCode == STILL_ACTIVE;
    } else {
        printf("GetExitCodeProcess failed (%d).\n", GetLastError());
        return FALSE;
    }
}

