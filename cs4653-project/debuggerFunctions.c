#include "debuggerFunctions.h"

// Function to check if a debugger is present
void checkDebugger() {
    if (IsDebuggerPresent()) {
        printf("Debugger detected! Exiting game.\n");
        exit(1); // Terminate the program if a debugger is detected
    }
}

void checkTiming(DWORD startTime) {
    DWORD currentTime = GetTickCount64();
    DWORD elapsedTime = currentTime - startTime;

    if (elapsedTime > 15) {
        printf("Abnormal delay detected! Exiting Game.\n");
        exit(1);
    }

}

#define STATUS_INFO_LENGTH_MISMATCH 0xC0000004
#define MAX_SUSPICIOUS_HANDLES 5

typedef unsigned char bool;
#define true 1
#define false 0

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX {
    PVOID Object;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR HandleValue;
    ULONG GrantedAccess;
    USHORT CreatorBackTraceIndex;
    USHORT ObjectTypeIndex;
    ULONG HandleAttributes;
    ULONG Reserved;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX, * PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX;

typedef struct _SYSTEM_HANDLE_INFORMATION_EX {
    ULONG_PTR NumberOfHandles;
    ULONG_PTR Reserved;
    SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handles[1];
} SYSTEM_HANDLE_INFORMATION_EX, * PSYSTEM_HANDLE_INFORMATION_EX;

typedef NTSTATUS(WINAPI* _NtQuerySystemInformation)(
    SYSTEM_INFORMATION_CLASS SystemInformationClass,
    PVOID                    SystemInformation,
    ULONG                    SystemInformationLength,
    PULONG                   ReturnLength
    );

// SystemHandleInformationEx might not be available on all Windows versions
#define SystemExtendedHandleInformation 0x40

bool checkDebuggerHandleScan() {
    DWORD myPid = GetCurrentProcessId();
    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (!hNtDll) return false;

    _NtQuerySystemInformation NtQuerySystemInformation = (_NtQuerySystemInformation)GetProcAddress(hNtDll, "NtQuerySystemInformation");
    if (!NtQuerySystemInformation) return false;

    ULONG bufferSize = 0x10000;
    PSYSTEM_HANDLE_INFORMATION_EX handleInfo = NULL;
    NTSTATUS status;
    ULONG returnLength;

    do {
        handleInfo = (PSYSTEM_HANDLE_INFORMATION_EX)malloc(bufferSize);
        if (!handleInfo) return false;

        status = NtQuerySystemInformation(SystemExtendedHandleInformation, handleInfo, bufferSize, &returnLength);
        if (status == STATUS_INFO_LENGTH_MISMATCH) {
            free(handleInfo);
            handleInfo = NULL;
            bufferSize = returnLength;
        }
    } while (status == STATUS_INFO_LENGTH_MISMATCH);

    if (!NT_SUCCESS(status)) {
        if (handleInfo) free(handleInfo);
        return false;
    }

    int suspiciousCount = 0;
    printf("Checking handles for suspicious activity...\n");

    for (ULONG_PTR i = 0; i < handleInfo->NumberOfHandles; i++) {
        SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX entry = handleInfo->Handles[i];

        if (entry.UniqueProcessId != myPid) {
            HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, (DWORD)entry.UniqueProcessId);
            if (hProcess) {
                HANDLE hDup;
                if (DuplicateHandle(hProcess, (HANDLE)entry.HandleValue,
                    GetCurrentProcess(), &hDup,
                    0, FALSE, DUPLICATE_SAME_ACCESS)) {
                    if (GetProcessId(hDup) == myPid) {
                        printf("Suspicious handle found: PID=%lu, Access=%08lx\n",
                            (DWORD)entry.UniqueProcessId, entry.GrantedAccess);
                        suspiciousCount++;
                    }
                    CloseHandle(hDup);
                }
                CloseHandle(hProcess);
            }
        }
    }

    free(handleInfo);

    if (suspiciousCount > MAX_SUSPICIOUS_HANDLES) {
        printf("Debugger detected: %d suspicious handles found\n", suspiciousCount);

        // Add your termination logic here
        MessageBoxA(NULL, "Debugger detected!", "Anti-Debug", MB_ICONERROR | MB_OK);
        ExitProcess(0);  // Immediate termination
        return true;
    }

    return false;
}