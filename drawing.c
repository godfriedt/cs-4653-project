#include "drawing.h"
#include "cards.h"
#include <math.h>
#include <raylib.h>
#include <stddef.h>
#include <stdio.h>

#define EVENT_QUEUE_SIZE 128

const int WORLD_WIDTH = 640;
const int WORLD_HEIGHT = 360;

// Real window size (px)
static int window_width = WORLD_WIDTH;
static int window_height = WORLD_HEIGHT;

const float CARD_HEIGHT = 77.0;
const float CARD_WIDTH = 52.0;

static int init = 0;
static Texture2D card_atlas;
static float scale = 4.0;

Vector2 screen_to_world(Vector2 screen_position) {
  screen_position.x /= scale;
  screen_position.y /= scale;
  return screen_position;
}

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
  Rectangle dst_rect = (Rectangle){.x = position.x * scale,
                                   .y = position.y * scale,
                                   .width = CARD_WIDTH * scale * flip,
                                   .height = CARD_HEIGHT * scale};
  DrawTexturePro(card_atlas, src_rect, dst_rect,
                 (Vector2){.x = CARD_WIDTH / 2.0 * flip * scale,
                           .y = CARD_HEIGHT / 2.0 * scale},
                 rotation, RAYWHITE);
}

const Vector2 ZERO = (Vector2){0, 0};
const float FLIP_SPEED = 8.0;
const float MOVE_SPEED = 4.0;
const float ROTATE_SPEED = 4.0;

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
  } variant;
} Event;
static Event event_queue[EVENT_QUEUE_SIZE];
static size_t event_queue_start = 0;
static size_t event_queue_end = 0;
static float last_event_start = 0;

const Vector2 DECK_POSITION = {(float)WORLD_WIDTH / 2 - 200,
                               (float)WORLD_HEIGHT / 2};

const Vector2 HAND_POSITIONS[4][2] = {
    {{0, (float)WORLD_HEIGHT / 2 - CARD_WIDTH / 2},
     {0, (float)WORLD_HEIGHT / 2 + CARD_WIDTH / 2}},

    {{(float)WORLD_WIDTH / 2 - CARD_WIDTH / 2, 0},
     {(float)WORLD_WIDTH / 2 + CARD_WIDTH / 2, 0}},

    {{WORLD_WIDTH, (float)WORLD_HEIGHT / 2 - CARD_WIDTH / 2},
     {WORLD_WIDTH, (float)WORLD_HEIGHT / 2 + CARD_WIDTH / 2}},

    {{(float)WORLD_WIDTH / 2 - CARD_WIDTH / 2, WORLD_HEIGHT},
     {(float)WORLD_WIDTH / 2 + CARD_WIDTH / 2, WORLD_HEIGHT}},
};

void reset_board() {
  for (int i = 0; i < CARD_COUNT; i++) {
    card_positions[i] = card_old_positions[i] = card_new_positions[i] =
        DECK_POSITION;
    card_flips[i] = card_old_flips[i] = card_new_flips[i] = -1.0;
    card_rotations[i] = card_old_rotations[i] = card_new_rotations[i] = 0;
    animation_start[i] = GetTime();
  }
  for (int i = 0; i < CARD_COUNT; i += 4) {
    queue_anim_move(i, DECK_POSITION, 180, -1.0);
    queue_anim_wait(0.03);
  }
}

void init_drawing() {
  window_width = 1920;
  window_height = 1080;
  scale = (float)window_width / (float)WORLD_WIDTH;
  InitWindow(window_width, window_height, "Hold'em");
  SetWindowTitle("Rowdy Hold'em");
  card_atlas = LoadTexture("res/cards_sheet.png");
  for (int i = 0; i < CARD_COUNT; i++) {
    card_positions[i] = card_old_positions[i] = card_new_positions[i] =
        DECK_POSITION;
    card_flips[i] = card_old_flips[i] = card_new_flips[i] = -1.0;
    card_rotations[i] = card_old_rotations[i] = card_new_rotations[i] = 180;
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

void queue_anim_move(size_t card, Vector2 position, float rotation,
                     float flip) {
  event_queue_end += 1;
  event_queue_end %= EVENT_QUEUE_SIZE;
  if (event_queue_end == event_queue_start) {
    exec_now(event_queue_start);
    event_queue_start += 1;
    event_queue_start %= EVENT_QUEUE_SIZE;
  }
  Event event = (Event){.tag = 0,
                        .variant.move = (MoveEvent){.card = card,
                                                    .position = position,
                                                    .rotation = rotation,
                                                    .flip = flip}};
  event_queue[event_queue_end] = event;
}

void queue_anim_wait(float time) {
  event_queue_end += 1;
  event_queue_end %= EVENT_QUEUE_SIZE;
  if (event_queue_end == event_queue_start) {
    exec_now(event_queue_start);
    event_queue_start += 1;
    event_queue_start %= EVENT_QUEUE_SIZE;
  }
  Event event = (Event){.tag = 1, .variant.wait = time};
  event_queue[event_queue_end] = event;
}

void draw_text(char *text, Vector2 position, int font_size, Color color) {
  position.x *= scale;
  position.y *= scale;
  font_size *= scale;
  // Center text
  Vector2 size = MeasureTextEx(GetFontDefault(), text, font_size, 4 * scale);
  position.x -= size.x / 2;
  position.y -= size.y / 2;
  // Draw text
  DrawTextEx(GetFontDefault(), text, position, font_size, 4 * scale, color);
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
    }
  }
  // Process ongoing animations
  for (int i = 0; i < CARD_COUNT; i++) {
    card_flips[i] = lerp_float(card_old_flips[i], card_new_flips[i],
                               animation_start[i], FLIP_SPEED, 1);
    card_positions[i] = lerp_v2(card_old_positions[i], card_new_positions[i],
                                animation_start[i], MOVE_SPEED, 0.5);
    card_rotations[i] = lerp_float(card_old_rotations[i], card_new_rotations[i],
                                   animation_start[i], ROTATE_SPEED, 0.25);
  }
  // Draw all objects
  ClearBackground(GREEN);
  // Draw moving cards first as a hack
  for (int i = 0; i < CARD_COUNT; i++) {
    draw_card(deck[i], card_positions[i], card_rotations[i], card_flips[i]);
  }
}
