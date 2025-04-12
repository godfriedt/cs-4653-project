#include "drawing.h"
#include "cards.h"
#include <math.h>
#include <raylib.h>
#include <stddef.h>
#include <stdio.h>

#define EVENT_QUEUE_SIZE 256
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

HandValue display_hand = 0;

static RenderTexture2D canvas;

static int init = 0;
static Texture2D card_atlas;

void draw_card(Card card, Vector2 position, float rotation, float flip) {
  if (card == 0 || init == 0)
    return;
  if (flip < 0.0) {
    card = BACKFACE;
    flip = -flip;
  }

  int face = get_face(card) - 1;
  int suite = (get_suite(card) >> 8) - 1;

  Rectangle src_rect = (Rectangle){.x = face * CARD_WIDTH,
                                   .y = suite * CARD_HEIGHT,
                                   .width = CARD_WIDTH,
                                   .height = CARD_HEIGHT};
  Rectangle dst_rect = (Rectangle){.x = position.x,
                                   .y = position.y,
                                   .width = CARD_WIDTH * flip,
                                   .height = CARD_HEIGHT};
  DrawTexturePro(
      card_atlas, src_rect, dst_rect,
      (Vector2){.x = CARD_WIDTH / 2.0 * flip, .y = CARD_HEIGHT / 2.0}, rotation,
      RAYWHITE);
}

const Vector2 ZERO = {0, 0};
const float FLIP_SPEED = 8.0;
const float MOVE_SPEED = 4.0;
const float ROTATE_SPEED = 6.0;

// App state
static float frame_time = 1.0 / 60.0;
static float current_time = 0.0;
// Game state
static Vector2 card_positions[CARD_COUNT];
static float card_rotations[CARD_COUNT];
static float card_flips[CARD_COUNT];
// Animation state
static float animation_start[CARD_COUNT];
static Vector2 card_old_positions[CARD_COUNT];
static Vector2 card_new_positions[CARD_COUNT];
static float card_old_rotations[CARD_COUNT];
static float card_new_rotations[CARD_COUNT];
static float card_old_flips[CARD_COUNT];
static float card_new_flips[CARD_COUNT];

static int32_t display_moneys[5] = {};
static int32_t old_display_moneys[5] = {};
static int32_t new_display_moneys[5] = {};
static float money_animation_start[5] = {};

// Animation event queue
typedef struct {
  size_t card;
  Vector2 position;
  float rotation;
  float flip;
} MoveEvent;
typedef struct {
  uint8_t tag;
  union {
    MoveEvent move;
    float wait;
    struct {
      size_t wallet;
      int16_t amount;
    } money;
  } variant;
} Event;
static Event event_queue[EVENT_QUEUE_SIZE];
static size_t event_queue_start = 0;
static size_t event_queue_end = 0;
static float last_event_start = 0;

const Vector2 DECK_POSITION = {(float)WORLD_WIDTH / 2 - 200,
                               (float)WORLD_HEIGHT / 2};

const Vector2 HAND_POSITIONS[4][2] = {
    {{0, (float)WORLD_HEIGHT / 2 + CARD_WIDTH / 2},
     {0, (float)WORLD_HEIGHT / 2 - CARD_WIDTH / 2}},

    {{(float)WORLD_WIDTH / 2 + CARD_WIDTH / 2, 0},
     {(float)WORLD_WIDTH / 2 - CARD_WIDTH / 2, 0}},

    {{WORLD_WIDTH, (float)WORLD_HEIGHT / 2 + CARD_WIDTH / 2},
     {WORLD_WIDTH, (float)WORLD_HEIGHT / 2 - CARD_WIDTH / 2}},

    {{(float)WORLD_WIDTH / 2 + CARD_WIDTH / 2, WORLD_HEIGHT},
     {(float)WORLD_WIDTH / 2 - CARD_WIDTH / 2, WORLD_HEIGHT}},
};

void init_drawing() {
  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(WORLD_WIDTH, WORLD_HEIGHT, "Hold'em");
  SetWindowTitle("Rowdy Hold'em");
  canvas = LoadRenderTexture(WORLD_WIDTH, WORLD_HEIGHT);
  card_atlas = LoadTexture("res/cards_sheet.png");
  for (int i = 0; i < CARD_COUNT; i++) {
    card_positions[i] = card_old_positions[i] = card_new_positions[i] =
        DECK_POSITION;
    card_flips[i] = card_old_flips[i] = card_new_flips[i] = -1.0;
    card_rotations[i] = card_old_rotations[i] = card_new_rotations[i] = 0;
    animation_start[i] = GetTime();
  }
  last_event_start = GetTime();
  frame_time = GetFrameTime();
  current_time = GetTime();
  init = 1;
}

float lerp_float(float start, float end, float delta, float speed,
                 float degree) {
  delta = pow((current_time - delta) * speed, degree);
  if (delta > 1.0)
    delta = 1.0;
  if (delta < 0.0)
    delta = 0.0;
  return (start + (end - start) * (delta * delta));
}
Vector2 lerp_v2(Vector2 start, Vector2 end, float delta, float speed,
                float degree) {
  return (Vector2){lerp_float(start.x, end.x, delta, speed, degree),
                   lerp_float(start.y, end.y, delta, speed, degree)};
}

void exec_now(size_t queue_position) {
  switch (event_queue[queue_position].tag) {
  case 0: {
    MoveEvent ev = event_queue[queue_position].variant.move;
    card_old_positions[ev.card] = card_new_positions[ev.card] =
        card_positions[ev.card] = ev.position;

    card_old_rotations[ev.card] = card_new_rotations[ev.card] =
        card_rotations[ev.card] = ev.rotation;

    card_old_flips[ev.card] = card_new_flips[ev.card] = card_flips[ev.card] =
        ev.flip;
    break;
  }
  case 1: {
    break;
  }
  }
}

void queue_anim(Event e) {
  event_queue_end += 1;
  event_queue_end %= EVENT_QUEUE_SIZE;
  if (event_queue_end == event_queue_start) {
    exec_now(event_queue_start);
    event_queue_start += 1;
    event_queue_start %= EVENT_QUEUE_SIZE;
  }
  event_queue[event_queue_end] = e;
}

int is_animations_finished() { return event_queue_start == event_queue_end; }

void queue_anim_move(size_t card, Vector2 position, float rotation,
                     float flip) {
  queue_anim((Event){.tag = 0,
                     .variant.move = (MoveEvent){.card = card,
                                                 .position = position,
                                                 .rotation = rotation,
                                                 .flip = flip}});
}
void flip_card(size_t card) {
  queue_anim_move(card, card_new_positions[card], card_new_rotations[card],
                  -card_flips[card]);
}

void queue_anim_money(size_t wallet, uint16_t new_amount) {
  queue_anim((Event){.tag = 2, .variant.money = {wallet, new_amount}});
};

void queue_anim_wait(float time) {
  queue_anim((Event){.tag = 1, .variant.wait = time});
}

void draw_text(char *text, Vector2 position, int font_size, Color color) {
  Vector2 size = MeasureTextEx(GetFontDefault(), text, font_size, 4);
  // Draw text
  DrawTextEx(GetFontDefault(), text, position, font_size, 4, color);
}

void draw_text_centered(char *text, Vector2 position, int font_size,
                        Color color) {
  Vector2 size = MeasureTextEx(GetFontDefault(), text, font_size, 4);
  position.x -= size.x / 2;
  position.y -= size.y / 2;
  // Draw text
  DrawTextEx(GetFontDefault(), text, position, font_size, 4, color);
}

ButtonState button_choice = NoButton;

float bet_spinner_value = 0;

void draw_ui() {
  Vector2 center = {(float)WORLD_WIDTH * 0.8, (float)WORLD_HEIGHT * 0.85};
  Vector2 size = {200, 75};
  // Panel
  Rectangle bounds = {.x = center.x - size.x / 2,
                      .y = center.y - size.y / 2,
                      .width = size.x,
                      .height = size.y};
  GuiPanel(bounds, NULL);
  // Buttons
  Rectangle bet_button_bounds = {.x = bounds.x,
                                 .y = bounds.y,
                                 .width = bounds.width / 3,
                                 .height = bounds.height / 2};
  if (GuiButton(bet_button_bounds, "BET"))
    button_choice = BetButton;
  Rectangle call_button_bounds = {.x = bounds.x + bounds.width / 3,
                                  .y = bounds.y,
                                  .width = bounds.width / 3,
                                  .height = bounds.height / 2};
  if (GuiButton(call_button_bounds, "CALL"))
    button_choice = CallButton;
  Rectangle fold_button_bounds = {.x = bounds.x + bounds.width / 3 * 2,
                                  .y = bounds.y,
                                  .width = bounds.width / 3,
                                  .height = bounds.height / 2};
  if (GuiButton(fold_button_bounds, "FOLD"))
    button_choice = FoldButton;
  // Slider
  Rectangle slider_bounds = {bounds.x, bounds.y + bounds.height / 2.0,
                             .width = bounds.width,
                             .height = bounds.height / 2};
  GuiSlider(slider_bounds, NULL, NULL, &bet_spinner_value, 0.0, 2000.0);
  bet_spinner_value = roundf(bet_spinner_value / 10.0) * 10.0;
  char buffer[64] = {};
  snprintf(buffer, 64, "$%d", (int)bet_spinner_value);
  GuiDrawText(buffer, slider_bounds, 1, BLACK);
}

void draw() {
  current_time = GetTime();
  // Process event queue
  if (event_queue_start != event_queue_end) {
    size_t current_index = (event_queue_start + 1) % EVENT_QUEUE_SIZE;
    Event current_ev = event_queue[current_index];
    switch (current_ev.tag) {
    case 0: {
      MoveEvent ev = current_ev.variant.move;
      card_old_positions[ev.card] = card_positions[ev.card];
      card_new_positions[ev.card] = ev.position;

      card_old_rotations[ev.card] = card_rotations[ev.card];
      card_new_rotations[ev.card] = ev.rotation;

      card_old_flips[ev.card] = card_flips[ev.card];
      card_new_flips[ev.card] = ev.flip;

      animation_start[ev.card] = current_time;
      event_queue_start = current_index;
      last_event_start = current_time;
      break;
    }
    case 1: {
      float wait_time = current_ev.variant.wait;
      if (last_event_start + wait_time <= current_time) {
        event_queue_start = current_index;
        last_event_start = current_time;
      }
      break;
    }
    case 2: {
      uint16_t wallet = current_ev.variant.money.wallet;
      uint16_t amount = current_ev.variant.money.amount;
      old_display_moneys[wallet] = display_moneys[wallet];
      new_display_moneys[wallet] = amount;
      money_animation_start[wallet] = current_time;
      event_queue_start = current_index;
      last_event_start = current_time;
      break;
    }
    }
  }
  // Card animations
  for (int i = 0; i < CARD_COUNT; i++) {
    card_flips[i] = lerp_float(card_old_flips[i], card_new_flips[i],
                               animation_start[i], FLIP_SPEED, 1);
    card_positions[i] = lerp_v2(card_old_positions[i], card_new_positions[i],
                                animation_start[i], MOVE_SPEED, 0.5);
    card_rotations[i] = lerp_float(card_old_rotations[i], card_new_rotations[i],
                                   animation_start[i], ROTATE_SPEED, 0.25);
  }
  // Money animations
  for (int i = 0; i < 5; i++) {
    display_moneys[i] =
        (int16_t)lerp_float(old_display_moneys[i], new_display_moneys[i],
                            money_animation_start[i], 2.5, 1.0);
  }
  BeginDrawing();
  BeginTextureMode(canvas);
  ClearBackground(GREEN);
  // Draw cards
  for (int i = 0; i < CARD_COUNT; i++) {
    draw_card(face_values[i], card_positions[i], card_rotations[i],
              card_flips[i]);
  }
  // Draw player moneys
  char buffer[64] = {};
  snprintf(buffer, 64, "P1:  $%d\nP2:  $%d\nP3:  $%d\nYou: $%d\n",
           display_moneys[0], display_moneys[1], display_moneys[2],
           display_moneys[3]);
  draw_text(buffer, (Vector2){10, (float)WORLD_HEIGHT / 2 + CARD_HEIGHT}, 20,
            BLACK);
  // Draw pot money
  snprintf(buffer, 64, "Pot: $%d", display_moneys[4]);
  draw_text_centered(
      buffer, (Vector2){WORLD_WIDTH / 2.0, WORLD_HEIGHT / 2.0 - CARD_HEIGHT},
      20, BLACK);
  // Draw result
  if (display_hand != 0) {
    draw_text_centered(
        hand_value_string(display_hand),
        (Vector2){WORLD_WIDTH / 2.0, WORLD_HEIGHT / 2.0 + CARD_HEIGHT}, 20,
        BLACK);
  }
  // Gui
  draw_ui();
  EndTextureMode();
  Rectangle src = {0, 0, WORLD_WIDTH, -WORLD_HEIGHT};
  // Correct for window's aspect ratio
  Rectangle window = {0, 0, GetScreenWidth(), GetScreenHeight()};
  float window_ratio = window.width / window.height;
  if (window_ratio >= 16.0 / 9.0) {
    // Too wide
    float new_width = (window.height / 9.0) * 16.0;
    window.x += (window.width - new_width) / 2.0;
    window.width = new_width;
  } else {
    // Too tall
    float new_height = (window.width / 16.0) * 9.0;
    window.y += (window.height - new_height) / 2.0;
    window.height = new_height;
  }
  ClearBackground(BLACK);
  DrawTexturePro(canvas.texture, src, window, (Vector2){0, 0}, 0, WHITE);
  EndDrawing();
}
