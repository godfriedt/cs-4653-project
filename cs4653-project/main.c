#include "cards.h"
#include "gameloop.h"
#include <raylib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// #include <Windows.h>

int main(void) {
  // checkDebugger(); // Check for debugger at the start of the program
  srand(time(NULL));
  start_gameloop();
  return 0;
}
