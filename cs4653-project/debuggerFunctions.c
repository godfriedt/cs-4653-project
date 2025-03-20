#include "debuggerFunctions.h"

// Function to check if a debugger is present
void checkDebugger() {
    if (IsDebuggerPresent()) {
        printf("Debugger detected! Exiting game.\n");
        exit(1); // Terminate the program if a debugger is detected
    }
}
