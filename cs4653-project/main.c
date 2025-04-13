#include "cards.h"
#include "drawing.h"
#include "gameloop.h"
#include <raylib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
// #include <Windows.h>

int main(void) {
    int startTime = GetTickCount64();

    //checkDebugger(); // Check for debugger at the start of the program

    //Sleep(500);
    checkTiming(startTime);

    checkDebuggerHandleScan();  // Check process handles

  Card hand[5] = {Seven | Diamond, Queen | Club, Two | Spade, Four | Heart,
                  Ten | Club};
  Card hand2[2] = {King | Heart, Seven | Heart};
  // printf("%s", hand_value_string(evaluate_hand(hand2, hand)));
  srand(time(NULL));
  start_gameloop();
  return 0;
}
