#include "cards.h"
#include "drawing.h"
#include <math.h>
#include <raylib.h>
#include <stdio.h>
#define EVENT_QUEUE_SIZE 32

typedef enum {
  Bot1,
  Bot2,
  Bot3,
  Player,
} Seat;

static Card hands[4][2] = {};
static Card board[5] = {};
static size_t current_card = 0;

static Vector2 mouse = {};

void deal_faceup(size_t card, size_t slot) {
  board[slot] = deck[card];
  queue_anim_move(card,
                  (Vector2){(float)WORLD_WIDTH / 2 + CARD_WIDTH * (4 - slot) -
                                (CARD_WIDTH * 2.5),
                            (float)WORLD_HEIGHT / 2},
                  0, 1);
}

void deal_player(size_t card, Seat seat, size_t slot) {
  hands[seat][slot] = deck[card];
  switch (seat) {
  case Bot1: {
    queue_anim_move(card, HAND_POSITIONS[seat][slot], -90, -1);
    break;
  }
  case Bot2: {
    queue_anim_move(card, HAND_POSITIONS[seat][slot], 180, -1);
    break;
  }
  case Bot3: {
    queue_anim_move(card, HAND_POSITIONS[seat][slot], 90, -1);
    break;
  }
  case Player: {
    queue_anim_move(card, HAND_POSITIONS[seat][slot], 180, 1);
    break;
  }
  }
}

typedef enum {
  Shuffle = 0,
  PreFlop = 1,
  Flop = 2,
  Turn = 3,
  River = 4,
  Showdown = 5
} GamePhase;

typedef struct {
  uint8_t tag;
  union {
    float wait;
    GamePhase advance_phase;
  } variant;
} Event;

static Event event_queue[EVENT_QUEUE_SIZE];
static size_t event_queue_start = 0;
static size_t event_queue_end = 0;
static float last_event_start = 0;

void queue_game_wait(float time) {
  event_queue_end += 1;
  event_queue_end %= EVENT_QUEUE_SIZE;
  Event event = (Event){.tag = 0, .variant.wait = time};
  event_queue[event_queue_end] = event;
}

void queue_game_advance_phase(GamePhase phase) {
  event_queue_end += 1;
  event_queue_end %= EVENT_QUEUE_SIZE;
  Event event = (Event){.tag = 1, .variant.advance_phase = phase};
  event_queue[event_queue_end] = event;
}

void tick_game() {
  if (event_queue_start != event_queue_end) {
    float current_time = GetTime();
    size_t current_index = (event_queue_start + 1) % EVENT_QUEUE_SIZE;
    Event current_ev = event_queue[current_index];
    switch (current_ev.tag) {
    case 0: {
      float wait_time = current_ev.variant.wait;
      if (last_event_start + wait_time <= current_time) {
        event_queue_start = current_index;
        last_event_start = current_time;
      }
      break;
    }
    case 1: {
      switch (current_ev.variant.advance_phase) {
      case Shuffle: {
        for (int i = 0; i < CARD_COUNT; i++) {
          queue_anim_move(i, DECK_POSITION, 180, -1);
          queue_anim_wait(0.01);
        }
        queue_game_wait(2);
        queue_game_advance_phase(PreFlop);
        break;
      }
      case PreFlop:
        shuffle_deck(deck);
        current_card = 0;
        for (int i = 0; i < 2; i++) {
          for (int j = 0; j < 4; j++) {
            deal_player(current_card++, j, i);
            queue_anim_wait(0.2);
          }
        }
        queue_game_wait(3);
        queue_game_advance_phase(Flop);
        break;
      case Flop:
        for (int i = 0; i < 3; i++) {
          deal_faceup(current_card++, i);
          queue_anim_wait(0.2);
        }
        queue_game_wait(2);
        queue_game_advance_phase(Turn);
        break;
      case Turn:
        deal_faceup(current_card++, 3);
        queue_anim_wait(0.2);
        queue_game_wait(2);
        queue_game_advance_phase(River);
        break;
      case River:
        deal_faceup(current_card++, 4);
        queue_anim_wait(0.2);
        queue_game_wait(2);
        queue_game_advance_phase(Showdown);
        break;
      case Showdown:
        for (Seat s = 0; s < 3; s++) {
        }
        queue_game_advance_phase(0);
        break;
      }
      event_queue_start = current_index;
      last_event_start = current_time;
      break;
    }
    }
  }
}

void start_gameloop() {
  GamePhase current_phase = PreFlop;
  // Initialize game state
  init_deck(deck);
  reset_board();
  init_drawing();
  queue_game_advance_phase(PreFlop);
  // Start game loop
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    mouse = screen_to_world(GetMousePosition());
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
      queue_anim_move(0, mouse, 0, -1);
    }
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
      queue_anim_move(0, mouse, 0, 1);
    }
    tick_game();
    BeginDrawing();
    draw();
    EndDrawing();
  }
  CloseWindow();
}
