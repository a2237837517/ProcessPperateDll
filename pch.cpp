// pch.cpp: 与预编译标头对应的源文件

#include "pch.h"


// 当使用预编译的头时，需要使用此源文件，编译才能成功。



extern "C" __declspec(dllexport) int __stdcall IsPortInUse(int port)
{
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    sockaddr_in service;
    int result = -1;

    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
        return -1;

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return -1;
    }

    service.sin_family = AF_INET;
    service.sin_addr.s_addr = htonl(INADDR_ANY);
    service.sin_port = htons(port);

    if (bind(sock, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        result = 1; // 端口被占用
    } else {
        result = -1; // 端口未被占用
        closesocket(sock);
    }

    WSACleanup();
    return result;
}




// 根据端口号获取进程ID
extern "C" __declspec(dllexport) DWORD  __stdcall GetProcessIdByPort(DWORD port) {
    PMIB_TCPTABLE2 pTcpTable = nullptr;
    ULONG size = 0;
    DWORD pid = 0;

    // 获取TCP连接表
    if (GetTcpTable2(nullptr, &size, TRUE) == ERROR_INSUFFICIENT_BUFFER) {
        pTcpTable = (PMIB_TCPTABLE2)malloc(size);
        if (pTcpTable == nullptr) {
            return 0;
        }
    }

    if (GetTcpTable2(pTcpTable, &size, TRUE) == NO_ERROR) {
        for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++) {
            // 检查本地端口是否匹配
            if (ntohs((u_short)pTcpTable->table[i].dwLocalPort) == port) {
                pid = pTcpTable->table[i].dwOwningPid;
                break;
            }
        }
    }

    if (pTcpTable != nullptr) {
        free(pTcpTable);
    }

    return pid;
}

// 根据进程ID获取进程名称
extern "C" __declspec(dllexport) std::string   __stdcall GetProcessNameById(DWORD pid) {
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return "";
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return "";
    }

    std::string processName;
    do {
        if (pe32.th32ProcessID == pid) {
            char processNameA[MAX_PATH];
            WideCharToMultiByte(CP_ACP, 0, pe32.szExeFile, -1,
                processNameA, MAX_PATH, NULL, NULL);
            processName = processNameA;
            break;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return processName;
}

// 终止指定进程ID的进程
extern "C" __declspec(dllexport) bool  __stdcall KillProcessById(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == nullptr) {
        return false;
    }

    bool result = TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
    return result;
}