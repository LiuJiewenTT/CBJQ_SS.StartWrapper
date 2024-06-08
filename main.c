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
BOOL ResolveSymbolicLink(wchar_t *szPath, wchar_t *szResolvedPath, DWORD dwResolvedPathSize);
BOOL StartProcessWithElevation(wchar_t *szResolvedPath, PROCESS_INFORMATION *pi);
BOOL RelaunchWithElevation(int argc, char *argv[]);

int main(int argc, char **argv) {
    if( argc < 2 ){
        return EXIT_FAILURE;
    }
    printf("arg[0]=%s\n", argv[0]);
    printf("arg[1]=%s\n", argv[1]);
    wchar_t *pw1 = NULL;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // 设置要启动的子进程路径，注意替换为实际要启动的程序路径
    wchar_t szCmdline[TEMPWSTR_LENGTH];
    wchar_t szResolvedPath[TEMPWSTR_LENGTH];
    pw1 = WCharChar(argv[1]);
    wcsncpy(szCmdline, pw1, TEMPWSTR_LENGTH);
    free2NULL(pw1);
    szCmdline[TEMPWSTR_LENGTH-1] = 0;

    // 解析符号链接到实际路径
    if (ResolveSymbolicLink(szCmdline, szResolvedPath, TEMPWSTR_LENGTH)) {
        wprintf(L"Resolved path: %s\n", szResolvedPath);
        wcscpy(szCmdline, szResolvedPath);
    } else {
        printf("Failed to resolve path or symbolic link.\n");
        return EXIT_FAILURE;
    }

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
        DWORD error = GetLastError();
        printf("CreateProcess failed (%d).\n", error);

        // 检查是否需要提升权限
        if (error == ERROR_ELEVATION_REQUIRED) {
            // printf("Attempting to start with elevation...\n");
            // if (!StartProcessWithElevation(szCmdline, &pi)) {
            //     printf("Failed to start with elevation.\n");
            //     return EXIT_FAILURE;
            // }

            printf("Attempting to elevate current process...\n");
            if (RelaunchWithElevation(argc, argv)) {
                printf("Relaunched process with elevated privileges worked well.\n");
                return EXIT_SUCCESS;
            } else {
                printf("Failed to relaunch with elevated privileges or relaunched process returned failed.\n");
                return EXIT_FAILURE;
            }
        } else {
            return EXIT_FAILURE;
        }
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


// 解析符号链接到实际路径
BOOL ResolveSymbolicLink(wchar_t *szPath, wchar_t *szResolvedPath, DWORD dwResolvedPathSize) {
    HANDLE hFile = CreateFile(
        szPath,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("CreateFile failed (%d).\n", GetLastError());
        return FALSE;
    }

    DWORD dwRet = GetFinalPathNameByHandle(hFile, szResolvedPath, dwResolvedPathSize, FILE_NAME_NORMALIZED);
    if (dwRet == 0 || dwRet > dwResolvedPathSize) {
        printf("GetFinalPathNameByHandle failed (%d).\n", GetLastError());
        CloseHandle(hFile);
        return FALSE;
    }

    // 去掉前缀 "\\?\"（如果存在）
    if (wcsncmp(szResolvedPath, L"\\\\?\\", 4) == 0) {
        wcscpy_s(szResolvedPath, dwResolvedPathSize, szResolvedPath + 4);
    }

    CloseHandle(hFile);
    return TRUE;
}


// 以提升权限运行子进程
BOOL StartProcessWithElevation(wchar_t *szResolvedPath, PROCESS_INFORMATION *pi) {
    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = szResolvedPath;
    sei.hwnd = NULL;
    sei.nShow = SW_NORMAL;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;  // 请求进程句柄

    if (!ShellExecuteEx(&sei)) {
        printf("ShellExecuteEx failed (%d).\n", GetLastError());
        return FALSE;
    }

    printf("Process started with elevation.\n");

    // 更新 PROCESS_INFORMATION 结构体
    if (pi != NULL && sei.hProcess != NULL) {
        pi->hProcess = sei.hProcess;
        pi->dwProcessId = GetProcessId(sei.hProcess);
        // hThread 和 dwThreadId 无法通过 ShellExecuteEx 获取，因此保留为 0
        pi->hThread = NULL;
        pi->dwThreadId = 0;
    }

    return TRUE;
}


// 以提升权限运行当前进程
BOOL RelaunchWithElevation(int argc, char *argv[]) {
    wchar_t szPath[MAX_PATH];
    if (!GetModuleFileName(NULL, szPath, MAX_PATH)) {
        printf("GetModuleFileName failed (%d).\n", GetLastError());
        return FALSE;
    }

    // 构建命令行参数
    wchar_t cmdLine[TEMPWSTR_LENGTH] = L"";
    // wcscat(cmdLine, L"\"");
    // wcscat(cmdLine, szPath);
    // wcscat(cmdLine, L"\"");

    for (int i = 1; i < argc; ++i) {
        wcscat(cmdLine, L" ");
        wcscat(cmdLine, L"\"");
        wchar_t *arg = WCharChar(argv[i]);
        wcscat(cmdLine, arg);
        wcscat(cmdLine, L"\"");
        free2NULL(arg);
    }

    wprintf(L"relaunch args=%s\n", cmdLine);

    // SHELLEXECUTEINFO sei = { sizeof(sei) };
    // sei.lpVerb = L"runas";
    // sei.lpFile = szPath;
    // sei.lpParameters = cmdLine; 
    // sei.nShow = SW_SHOWNORMAL;
    // sei.fMask = SEE_MASK_NOCLOSEPROCESS; // 保持进程句柄

    // 设置新的启动信息
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(
            NULL,
            cmdLine,
            NULL,
            NULL,
            FALSE,
            CREATE_NEW_CONSOLE, // 保持在当前控制台
            NULL,
            NULL,
            &si,
            &pi)
    ) {
        printf("CreateProcess (elevated) failed (%d).\n", GetLastError());
        return FALSE;
    }

    // if (!ShellExecuteEx(&sei)) {
    //     printf("ShellExecuteEx failed (%d).\n", GetLastError());
    //     return FALSE;
    // }

    // return TRUE;

    // 等待新进程结束
    // WaitForSingleObject(sei.hProcess, INFINITE);
    WaitForSingleObject(pi.hProcess, INFINITE);

    // 获取新进程的返回值
    DWORD exitCode;
    // if (!GetExitCodeProcess(sei.hProcess, &exitCode)) {
    //     printf("GetExitCodeProcess failed (%d).\n", GetLastError());
    //     CloseHandle(sei.hProcess);
    //     return FALSE;
    // }
    if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
        printf("GetExitCodeProcess failed (%d).\n", GetLastError());
        CloseHandle(pi.hProcess);
        return FALSE;
    }

    // CloseHandle(sei.hProcess);
    CloseHandle(pi.hProcess);

    // 判断新进程的返回值
    if (exitCode == EXIT_SUCCESS) {
        return TRUE;
    } else {
        return FALSE;
    }
}

