#include "cards.h"
#include "drawing.h"
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
// Maximum number of events in the gameloop event queue
#define EVENT_QUEUE_SIZE 128
static size_t event_queue_start = 0;
static size_t event_queue_end = 0;
// Enum for seats at the table / the players in them
typedef enum {
  Bot1,
  Bot2,
  Bot3,
  Player,
} Seat;

// Game state data
static size_t hands[4][2] = {};
static int32_t money[5] = {1000, 1000, 1000, 1000, 0};
static int32_t current_bets[4] = {};
static bool folded[4] = {};
static size_t board[5] = {};
static size_t current_card = 0;

void fold(Seat who) { folded[who] = true; }

void call(Seat who) {
  int32_t max_bet = 0;
  for (int i = 0; i < 4; i++) {
    if (current_bets[i] > max_bet)
      max_bet = current_bets[i];
  }
  int32_t amount_to_call = max_bet - current_bets[who];
  if (amount_to_call > money[who] || folded[who]) {
    fold(who);
    return;
  }
  money[who] -= amount_to_call;
  queue_anim_money(who, money[who]);
  current_bets[who] += amount_to_call;
  money[4] += amount_to_call;
  queue_anim_money(4, money[4]);
}

// Player `who` bets `amount` to the pot. This checks to prevent betting more
// than you have, and prevents split pots by capping the bet to the poorest
// players money
void bet(Seat who, int32_t amount) {
  // Prevent betting more than poorest player still in the game
  call(who);
  if (folded[who])
    return;
  int32_t total_amount = money[4];
  for (size_t i = 0; i < 4; i++) {
    total_amount += money[i];
    int32_t amount_to_call = money[4] + amount - current_bets[i];
    if (amount_to_call > money[i] && !folded[i]) {
      amount -= amount_to_call - money[i];
      amount = money[i];
    }
  }
  if (amount <= 0)
    return;
  // Check for invalid money amount
  if (total_amount != 4000)
    event_queue_start = event_queue_end;
  money[who] -= amount;
  current_bets[who] += amount;
  queue_anim_money(who, money[who]);
  money[4] += amount;
  queue_anim_money(4, money[4]);
}

void all_in(Seat who) { bet(who, money[who]); }

// Move all money in the pot to specified player
void payout(Seat who) {
  money[who] += money[4];
  queue_anim_money(who, money[who]);
  money[4] = 0;
  queue_anim_money(4, 0);
}

// Deal card onto the board
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
  // Cards move back to origin position, face values are shuffled
  Shuffle = 0,
  // Players are dealt hands, initial betting
  PreFlop = 1,
  // First three cards are dealt to table, betting
  Flop = 2,
  // Fourth card is dealt to table, betting
  Turn = 3,
  // Fifth card is dealt to table, betting
  River = 4,
  // Hands are revealed, payout to winner
  Showdown = 5
} GamePhase;

static GamePhase current_phase = Shuffle;

// Event object
typedef struct {
  enum { AdvancePhase, AdvanceTurn } tag;
  union {
    GamePhase next_phase;
    Seat next_turn;
  } variant;
} Event;
// Event loop state
static Event event_queue[EVENT_QUEUE_SIZE];
static float last_event_start = 0;

void queue_event(Event event) {
  event_queue_end += 1;
  event_queue_end %= EVENT_QUEUE_SIZE;
  event_queue[event_queue_end] = event;
}

void queue_game_phase(GamePhase phase) {
  queue_event((Event){.tag = AdvancePhase, .variant.next_phase = phase});
}

void queue_next_game_phase() {
  switch (current_phase) {
  case Shuffle:
    queue_game_phase(PreFlop);
    break;
  case PreFlop:
    queue_game_phase(Flop);
    break;
  case Flop:
    queue_game_phase(Turn);
    break;
  case Turn:
    queue_game_phase(River);
    break;
  case River:
    queue_game_phase(Showdown);
    break;
  case Showdown:
    queue_game_phase(Shuffle);
    break;
  }
}

void queue_turn_order() {
  // Do not queue turns if someone is all in
  for (int i = 0; i < 4; i++) {
    if (!folded[i]) {
      queue_event((Event){.tag = AdvanceTurn, .variant.next_turn = i});
    }
  }
}

// The core game loop: execute the next event and pop it if finished
void tick_game() {
  if (event_queue_start == event_queue_end) {
    is_caught = 1;
  } else {
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
      for (int i = 0; i < 5; i++)
        current_bets[i] = 0;
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
        display_hand = 0;
        queue_game_phase(PreFlop);
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
        break;
      case Flop:
        for (int i = 0; i < 3; i++) {
          deal_faceup(current_card++, i);
          queue_anim_wait(0.2);
        }
        queue_turn_order();
        break;
      case Turn:
        deal_faceup(current_card++, 3);
        queue_turn_order();
        break;
      case River:
        deal_faceup(current_card++, 4);
        queue_turn_order();
        break;
      case Showdown: {
        HandValue max_value = 0;
        size_t winning_player = Player;
        Card this_board[5] = {
            face_values[board[0]], face_values[board[1]], face_values[board[2]],
            face_values[board[3]], face_values[board[4]],
        };
        for (int i = 0; i < Player; i++) {
          flip_card(hands[i][0]);
          flip_card(hands[i][1]);
          queue_anim_wait(0.2);
          if (!folded[i]) {
            Card this_hand[2] = {face_values[hands[i][0]],
                                 face_values[hands[i][1]]};
            HandValue this_value = evaluate_hand(this_hand, this_board);
            if (this_value > max_value) {
              max_value = this_value;
              winning_player = i;
            }
          }
        }
        display_hand = max_value;
        payout(winning_player);
        queue_anim_wait(5);
        queue_next_game_phase();
        break;
      }
      }
      break;
    }
    case AdvanceTurn: {
      Seat current_seat = current_ev.variant.next_turn;
      if (folded[current_seat]) {
        printf("Break\n");
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
        call(current_seat);
      } else {
        // Player + last turn
        switch (button_choice) {
        case NoButton:
          return;
        case BetButton:
          bet(current_seat, bet_spinner_value);
          break;
        case CallButton:
          call(current_seat);
          break;
        case FoldButton:
          fold(current_seat);
          break;
        }
        button_choice = NoButton;
        // Determine if turn order should repeat
        bool do_next_turn = false;
        int32_t max_bet = 0;
        for (int i = 0; i < 4; i++)
          if (current_bets[i] > max_bet && !folded[i])
            max_bet = current_bets[i];
        for (int i = 0; i < 4; i++)
          if (current_bets[i] < max_bet && !folded[i])
            do_next_turn = true;
        if (do_next_turn) {
          queue_turn_order();
        } else {
          queue_next_game_phase();
        }
      }
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
  for (int i = 0; i < 4; i++) {
    queue_anim_money(i, money[i]);
  }
  queue_game_phase(Shuffle);
  // Start game loop
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    tick_game();
    draw();
  }
  CloseWindow();
}
