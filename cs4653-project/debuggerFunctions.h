#ifndef DEBUGGER_FUNCTIONS_H
#define DEBUGGER_FUNCTIONS_H

#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <winternl.h>
#pragma comment(lib, "ntdll.lib")

// Function to check if a debugger is present
void checkDebugger();

void checkTiming(DWORD startTime);

// Function to check how many processes have a handle on the current process
void checkDebuggerHandleScan();

void execute_anti_disassembly(void);

#endif // DEBUGGER_FUNCTIONS_H
