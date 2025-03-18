#include "cards.h"
#include "drawing.h"
#include <raylib.h>
#include <stdbool.h>
#include <stdlib.h>
#define EVENT_QUEUE_SIZE 128

typedef enum {
  Bot1,
  Bot2,
  Bot3,
  Player,
} Seat;

static size_t hands[4][2] = {};
static int32_t money[5] = {1000, 1000, 1000, 1000, 0};
static bool folded[4] = {};
static size_t board[5] = {};
static size_t current_card = 0;

static Vector2 mouse = {};

void bet(Seat who, int32_t amount) {
  // Prevent betting more than poorest player still in the game
  for (size_t i = 0; i < 4; i++) {
    if (amount > money[i] && !folded[i]) {
      amount = money[i];
    }
  }
  money[who] -= amount;
  queue_anim_money(who, money[who]);
  money[4] += amount;
  queue_anim_money(4, money[4]);
}

void all_in(Seat who) { bet(who, money[who]); }

void fold(Seat who) { folded[who] = true; }

void payout(Seat who) {
  money[who] += money[4];
  queue_anim_money(who, money[who]);
  money[4] = 0;
  queue_anim_money(4, 0);
}

void deal_faceup(size_t card, size_t slot) {
  board[slot] = card;
  queue_anim_move(
      card,
      (Vector2){(float)WORLD_WIDTH / 2 + CARD_WIDTH * slot - (CARD_WIDTH * 2),
                (float)WORLD_HEIGHT / 2},
      180, 1);
}

void deal_player(size_t card, Seat seat, size_t slot) {
  hands[seat][slot] = card;
  Vector2 pos = HAND_POSITIONS[seat][slot];
  switch (seat) {
  case Bot1: {
    queue_anim_move(card, pos, -90, -1);
    break;
  }
  case Bot2: {
    queue_anim_move(card, pos, 180, -1);
    break;
  }
  case Bot3: {
    queue_anim_move(card, pos, 90, -1);
    break;
  }
  case Player: {
    queue_anim_move(card, pos, 180, 1);
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

static GamePhase current_phase = Shuffle;

typedef struct {
  enum { AdvancePhase, AdvanceTurn } tag;
  union {
    GamePhase next_phase;
    Seat next_turn;
  } variant;
} Event;

static Event event_queue[EVENT_QUEUE_SIZE];
static size_t event_queue_start = 0;
static size_t event_queue_end = 0;
static float last_event_start = 0;

void queue_event(Event event) {
  event_queue_end += 1;
  event_queue_end %= EVENT_QUEUE_SIZE;
  event_queue[event_queue_end] = event;
}

void queue_game_advance_phase(GamePhase phase) {
  queue_event((Event){.tag = AdvancePhase, .variant.next_phase = phase});
}

void queue_turn_order() {
  // Do not queue turns if someone is all in
  for (int i = 0; i < 4; i++) {
    if (money[i] == 0 && !folded[i]) {
      return;
    }
  }
  for (int i = 0; i < 4; i++) {
    if (!folded[i]) {
      queue_event((Event){.tag = AdvanceTurn, .variant.next_turn = i});
    }
  }
}

void tick_game() {
  if (event_queue_start != event_queue_end) {
    float current_time = GetTime();
    size_t current_index = (event_queue_start + 1) % EVENT_QUEUE_SIZE;
    Event current_ev = event_queue[current_index];
    switch (current_ev.tag) {
    case AdvancePhase: {
      // Wait for animations to catch up
      if (!is_animations_finished()) {
        return;
      }
      current_phase = current_ev.variant.next_phase;
      switch (current_phase) {
      case Shuffle: {
        for (int i = 0; i < CARD_COUNT; i++) {
          queue_anim_move(i, DECK_POSITION, 0, -1);
          queue_anim_wait(0.01);
        }
        for (int i = 0; i < CARD_COUNT; i += 16) {
          queue_anim_move(i, DECK_POSITION, 180, -1.0);
          queue_anim_wait(0.03);
        }
        queue_game_advance_phase(PreFlop);
        break;
      }
      case PreFlop:
        shuffle_face_values();
        current_card = 0;
        for (int i = 0; i < 2; i++) {
          for (int j = 0; j < 4; j++) {
            deal_player(current_card++, j, i);
            queue_anim_wait(0.2);
          }
        }
        queue_turn_order();
        queue_game_advance_phase(Flop);
        break;
      case Flop:
        for (int i = 0; i < 3; i++) {
          deal_faceup(current_card++, i);
          queue_anim_wait(0.2);
        }
        queue_turn_order();
        queue_game_advance_phase(Turn);
        break;
      case Turn:
        deal_faceup(current_card++, 3);
        queue_turn_order();
        queue_game_advance_phase(River);
        break;
      case River:
        deal_faceup(current_card++, 4);
        queue_turn_order();
        queue_game_advance_phase(Showdown);
        break;
      case Showdown:
        for (int i = 0; i < Player; i++) {
          flip_card(hands[i][0]);
          flip_card(hands[i][1]);
          queue_anim_wait(0.2);
        }
        payout(Player);
        queue_game_advance_phase(0);
        break;
      }
      break;
    }
    case AdvanceTurn: {
      Seat current_seat = current_ev.variant.next_turn;
      if (folded[current_seat]) {
        break;
      }
      Card hand_values[2] = {face_values[hands[current_seat][0]],
                             face_values[hands[current_seat][1]]};
      Card board_values[5] = {
          face_values[board[0]], face_values[board[1]], face_values[board[2]],
          face_values[board[3]], face_values[board[4]],
      };
      // Blank out cards that aren't dealt yet
      if (current_phase < Flop)
        board_values[0] = board_values[1] = board_values[2] = 0;
      if (current_phase < Turn)
        board_values[3] = 0;
      if (current_phase < River)
        board_values[4] = 0;

      HandValue value = evaluate_hand(hand_values, board_values);

      if (current_seat != Player) {
        // AI strategy
        float random = (float)rand() / (float)(RAND_MAX);
      }
      bet(current_seat, 100);
      queue_anim_wait(1);
      break;
    }
    }
    event_queue_start = current_index;
    last_event_start = current_time;
  }
}

void start_gameloop() {
  // Initialize game state
  init_face_values();
  init_drawing();
  display_hand = RoyalFlush;
  for (int i = 0; i < 4; i++) {
    queue_anim_money(i, money[i]);
  }
  queue_game_advance_phase(Shuffle);
  // Start game loop
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    tick_game();
    draw();
  }
  CloseWindow();
}
