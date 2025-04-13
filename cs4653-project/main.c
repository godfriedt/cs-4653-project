#include "cards.h"
#include "gameloop.h"
#include <raylib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define _CRT_SECURE_NO_WARNINGS


// #include <Windows.h>

int main(void) {
	int startTime = GetTickCount64();

	// checkDebugger(); // Check for debugger at the start of the program

	//Sleep(500);
	checkTiming(startTime);

	checkDebuggerHandleScan();  // Check process handles

	srand(time(NULL));
	start_gameloop();
	return 0;
}
